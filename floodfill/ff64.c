#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image3d/binary.h"

static inline int mod(int a, int n) {
	return a>=n ? a%n : a>=0 ? a : n-1-(-1-a)%n;
}

static inline int memchr_off(const uint8_t *s, uint8_t c, size_t n) {
	uint8_t *po = memchr(s, c, n);
	if (po == NULL) {
		return -1;
	} else {
		return po - s;
	}
}

void floodfill_ff64(struct i3d_binary *restrict dst, struct i3d_binary *restrict src, int xs, int ys, int zs) {
	int s_xy = src->off_z;

	uint8_t seed_color = i3d_binary_at(src, xs, ys, zs);

	uint8_t *queue = calloc(s_xy * src->size_z, sizeof(uint8_t));
	queue[i3d_binary_offset(src, xs, ys, zs)] = 0xff;

	int cx = xs, cy = ys, cz = zs;

	while (1) {
		if (0 && !i3d_inside(src, cx, cy, cz)) {
			fprintf(stderr, "cc out of bounds: %d %d %d. This shouldn't happen.\n", cx, cy, cz);
			exit(1);
		}

		int rowstart = i3d_binary_offset(src, 0, cy, cz);
		queue[rowstart + cx] = 0;

		/* look for start of a line of pixels to be filled */
		while (cx >= 0
			&& src->voxels[rowstart + cx] == seed_color
			&& dst->voxels[rowstart + cx] != seed_color
		) {
			cx--;
		}
		cx++;

		/* fill them.
		 * limit is maximum length we may read starting from spos,
		 * which is initialized with, well, the current position.
		 *
		 * as spos is advanced, limit is decreased.
		 * we stop when either the limit is reached, or
		 * when a differently-colored pixel is found
		 */
		int spos = rowstart + cx;
		size_t limit = src->size_x - cx;

		/* first process all 8-byte-blocks that are to be completely filled.
		 * the neighbors that have to be queued can be calculated at once
		 */
		if (seed_color == 0xff) {
			while (limit >= 8) {
				uint64_t *sp = (uint64_t*)(src->voxels + spos);
				uint64_t *dp = (uint64_t*)(dst->voxels + spos);
				if ((~dp[0] & sp[0]) != UINT64_MAX) {
					break;
				}
				uint64_t *qp = (uint64_t*)(queue + spos);
				qp[0] = 0;
				dp[0] = UINT64_MAX;

				#define maybe_enqueue64(dy,dz) do { \
					uint64_t *s1p = (uint64_t*)(src->voxels + spos + src->off_y*dy + s_xy*dz); \
					uint64_t *d1p = (uint64_t*)(dst->voxels + spos + src->off_y*dy + s_xy*dz); \
					uint64_t *q1p = (uint64_t*)(queue + spos + src->off_y*dy + s_xy*dz); \
					q1p[0] |= (~d1p[0] & s1p[0]); \
				} while (0)
				if (cy > 0) maybe_enqueue64(-1,0);
				if (cz > 0) maybe_enqueue64(0,-1);
				if (cy < src->size_y - 1) maybe_enqueue64(+1,0);
				if (cz < src->size_z - 1) maybe_enqueue64(0,+1);
				limit -= 8;
				spos += 8;
			}
		} else {
			while (limit >= 8) {
				uint64_t *sp = (uint64_t*)(src->voxels + spos);
				uint64_t *dp = (uint64_t*)(dst->voxels + spos);
				if ((~sp[0] & dp[0]) != UINT64_MAX) {
					break;
				}
				uint64_t *qp = (uint64_t*)(queue + spos);
				qp[0] = 0;
				dp[0] = 0;

				#undef maybe_enqueue64
				#define maybe_enqueue64(dy,dz) do { \
					uint64_t *s1p = (uint64_t*)(src->voxels + spos + src->off_y*dy + s_xy*dz); \
					uint64_t *d1p = (uint64_t*)(dst->voxels + spos + src->off_y*dy + s_xy*dz); \
					uint64_t *q1p = (uint64_t*)(queue + spos + src->off_y*dy + s_xy*dz); \
					q1p[0] |= (~s1p[0] & d1p[0]); \
				} while (0)
				if (cy > 0) maybe_enqueue64(-1,0);
				if (cz > 0) maybe_enqueue64(0,-1);
				if (cy < src->size_y - 1) maybe_enqueue64(+1,0);
				if (cz < src->size_z - 1) maybe_enqueue64(0,+1);
				limit -= 8;
				spos += 8;
			}
		}

		while (limit > 0
			&& src->voxels[spos] == seed_color
			&& dst->voxels[spos] != seed_color
		) {
			dst->voxels[spos] = seed_color;
			queue[spos] = 0;
			#define maybe_enqueue(dy,dz) do { \
				int loloff = spos + dy*src->off_y + dz*s_xy; \
				if (src->voxels[loloff] == seed_color && dst->voxels[loloff] != seed_color) { \
					queue[loloff] = 0xff; \
				} \
			} while (0)
			if (cy > 0) maybe_enqueue(-1,0);
			if (cz > 0) maybe_enqueue(0,-1);
			if (cy < src->size_y - 1) maybe_enqueue(+1,0);
			if (cz < src->size_z - 1) maybe_enqueue(0,+1);
			spos++;
			limit--;
		}

		/* we filled from our current position.
		 *
		 * find a new start position now.
		 * we try in this order:
		 * - current line
		 * - following lines on same z
		 * - previous lines on same z
		 * - all z layers, starting with the one below, going up, wrapping around
		 */

		/* look on current line */
		cx = memchr_off(queue + rowstart, 0xff, src->size_x);
		if (cx != -1) {
			continue;
		}

		/* look on the following lines (if there are any) */
		int zstart = i3d_binary_offset(src, 0, 0, cz);
		if (cy + 1 < src->size_y) {
			int offs = memchr_off(queue + zstart + (cy+1)*src->off_y, 0xff, src->off_y * (src->size_y - (cy+1)));
			if (offs != -1) {
				cx = offs % src->off_y;
				cy = offs / src->off_y + (cy+1);
				continue;
			}
		}

		/* try on the lines before */
		if (cy > 0) {
			int offs = memchr_off(queue + zstart, 0xff, src->off_y * (cy-1));
			if (offs != -1) {
				cx = offs % src->off_y;
				cy = offs / src->off_y;
				continue;
			}
		}

		/* still not found, scan all the z now */
		int found = 0;
		for (int cz1 = mod(cz - 1, src->size_z); cz1 != mod(cz-2,src->size_z) && !found; cz1 = (cz1 + 1) % src->size_z) {
			if (cz1 == cz) { continue; }
			int offs = memchr_off(queue + cz1 * s_xy, 0xff, s_xy);
			if (offs != -1) {
				cx = offs % src->off_y;
				cy = offs / src->off_y;
				cz = cz1;
				found = 1; break;
			}
		}
		if (found) { continue; }

		/* still not found. so... queue empty \o/ */
		break;
	}
	free(queue);
}

