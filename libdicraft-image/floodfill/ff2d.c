#include "floodfill/ff2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void floodfill2d(binary_t dst, binary_t the_src, int xs, int ys, int z) {
	binary_t src;
	if (dst == the_src) {
		src = binary_like(the_src);
		memcpy(src->voxels, the_src->voxels, the_src->size_z * the_src->off_z);
	} else {
		src = the_src;
	}

	int s_xy = src->off_z;

	uint8_t seed_color = binary_at(src, xs, ys, z);

	uint8_t *queue = calloc(s_xy * src->size_z, sizeof(uint8_t));
	queue[binary_offset(src, xs, ys, z)] = 0xff;

	int cx = xs, cy = ys, cz = z;

	binary_t d_q = binary_like(the_src);
	free(d_q->voxels);
	d_q->voxels = queue;

	while (1) {
		if (!i3d_inside(src, cx, cy, cz)) {
			fprintf(stderr, "cc out of bounds: %d %d %d. This shouldn't happen.\n", cx, cy, cz);
			exit(1);
		}

		int rowstart = binary_offset(src, 0, cy, cz);
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

				#define maybe_enqueue64(dy) do { \
					uint64_t *s1p = (uint64_t*)(src->voxels + spos + src->off_y*dy); \
					uint64_t *d1p = (uint64_t*)(dst->voxels + spos + src->off_y*dy); \
					uint64_t *q1p = (uint64_t*)(queue + spos + src->off_y*dy); \
					q1p[0] |= (~d1p[0] & s1p[0]); \
				} while (0)
				if (cy > 0) maybe_enqueue64(-1);
				if (cy < src->size_y - 1) maybe_enqueue64(+1);
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
				#define maybe_enqueue64(dy) do { \
					uint64_t *s1p = (uint64_t*)(src->voxels + spos + src->off_y*dy); \
					uint64_t *d1p = (uint64_t*)(dst->voxels + spos + src->off_y*dy); \
					uint64_t *q1p = (uint64_t*)(queue + spos + src->off_y*dy); \
					q1p[0] |= (~s1p[0] & d1p[0]); \
				} while (0)
				if (cy > 0) maybe_enqueue64(-1);
				if (cy < src->size_y - 1) maybe_enqueue64(+1);
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
			#define maybe_enqueue(dy) do { \
				int loloff = spos + dy*src->off_y; \
				if (src->voxels[loloff] == seed_color && dst->voxels[loloff] != seed_color) { \
					queue[loloff] = 0xff; \
				} \
			} while (0)
			if (cy > 0) maybe_enqueue(-1);
			if (cy < src->size_y - 1) maybe_enqueue(+1);
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
		 */

		/* look on current line */
		cx = memchr_off(queue + rowstart, 0xff, src->size_x);
		if (cx != -1) {
			continue;
		}

		int offs = memchr_off(queue + cz*src->off_z, 0xff, src->off_y * src->size_y);
		if (offs != -1) {
			cx = offs % src->off_y;
			cy = offs / src->off_y;
			continue;
		}

		/* still not found. so... queue empty \o/ */
		break;
	}
	free(queue);

	if (dst == the_src) {
		free(src);
	}
}

