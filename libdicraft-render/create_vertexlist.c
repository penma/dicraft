#include "create_vertexlist.h"

#include "image3d/binary.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* Add a cube to given vertex+normal buffer, update the vertex count.
 * The buffer has to be large enough.
 */
static void cubedim(short xa, short ya, short za, short xb, short yb, short zb, short *vnbuf, int *vertcount) {
	short *vn = vnbuf + *vertcount * 2 * 3;

	short nx, ny, nz;
#   define V(a,b,c) \
	vn[0] = (a 1 > 0 ? xb : xa); \
	vn[1] = (b 1 > 0 ? yb : ya); \
	vn[2] = (c 1 > 0 ? zb : za); \
	vn[3] = nx; \
	vn[4] = ny; \
	vn[5] = nz; \
	vn += 6;
#   define N(a,b,c) nx = a; ny = b; nz = c;
	N( 1, 0, 0);
		V(+,-,+); V(+,-,-); V(+,+,+);
		V(+,-,-); V(+,+,-); V(+,+,+);
	N( 0, 1, 0);
		V(+,+,+); V(+,+,-); V(-,+,+);
		V(+,+,-); V(-,+,-); V(-,+,+);
	N( 0, 0, 1);
		V(+,+,+); V(-,+,+); V(+,-,+);
		V(-,+,+); V(-,-,+); V(+,-,+);
	N(-1, 0, 0);
		V(-,-,+); V(-,+,+); V(-,-,-);
		V(-,+,+); V(-,+,-); V(-,-,-);
	N( 0,-1, 0);
		V(-,-,+); V(-,-,-); V(+,-,+);
		V(-,-,-); V(+,-,-); V(+,-,+);
	N( 0, 0,-1);
		V(-,-,-); V(-,+,-); V(+,-,-);
		V(-,+,-); V(+,+,-); V(+,-,-);
#undef V
#undef N

	*vertcount += 6*2*3;
}

/* Append all voxels for this image to the given vertex+normal buffer.
 * Updates the vertex count. Expects the buffer to have enough space already.
 */
void append_vertices_for_image(binary_t im, short *vnbuf, int *vertcount) {
	/* for statistics */
	int pointsDrawn = 0;
	int cubesDrawn = 0;

	int startx = -1;
	int
		xm = im->size_x,
		ym = im->size_y,
		zm = im->size_z;

	for (int z = 0; z < zm; z++) {
		for (int y = 0; y < ym; y++) {
			for (int x = 0; x < xm; x++) {
				if (binary_at(im, x, y, z)) {
					pointsDrawn++;
					/* we're at the start, or in the middle of,
					 * a row of consecutive vertices.
					 */
					if (startx == -1) {
						startx = x;
					}
				} else if (startx != -1) {
					/* a row of consecutive vertices ends here.
					 * create a cube for them now
					 */
					cubedim(startx, y, z, x, y+1, z+1, vnbuf, vertcount);
					cubesDrawn++;
					startx = -1;
				}
			}

			/* if the row of vertices ends on the image border,
			 * we still need to create a cube
			 */
			if (startx != -1) {
				cubedim(startx, y, z, xm, y+1, z+1, vnbuf, vertcount);
				cubesDrawn++;
				startx = -1;
			}
		}
	}

	/* statistics */
	fprintf(stderr, "Rendered %7d points in %7d cubes (%7.4f%%)\n", pointsDrawn, cubesDrawn, 100. * cubesDrawn / pointsDrawn);
}

static inline int memchr_off(const uint8_t *s, uint8_t c, size_t n) {
	uint8_t *po = memchr(s, c, n);
	if (po == NULL) {
		return -1;
	} else {
		return po - s;
	}
}

/* Finds out the number of vertices needed to represent this image.
 */
int count_vertices(binary_t im) {
	int vertices = 0;

	int
		xm = im->size_x,
		ym = im->size_y,
		zm = im->size_z;

	for (int z = 0; z < zm; z++) {
		for (int y = 0; y < ym; y++) {
			int rest = xm;
			uint8_t *ptr = im->voxels + binary_offset(im, 0, y, z);
			int o;
			while (1) {
				o = memchr_off(ptr, 0xff, rest);
				if (o == -1) break;
				ptr += o + 1;
				rest -= o + 1;

				vertices += 6*2*3;

				o = memchr_off(ptr, 0x00, rest);
				if (o == -1) break;
				ptr += o + 1;
				rest -= o + 1;
			}
		}
	}

	return vertices;
}

