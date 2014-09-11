#include "create_vertexlist.h"

#include "image3d/binary.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Add a cube to given vertex+normal buffer, update the vertex count.
 * The buffer has to be large enough.
 */
static void cubedim(short xa, short ya, short za, short xb, short yb, short zb, short *vnbuf, int *vertcount) {
	short *vn = vnbuf + *vertcount * 2 * 3;

	if (xa >= xb){ fprintf(stderr, "LOL\n"); exit(1); }

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
	int
		xm = im->size_x,
		ym = im->size_y,
		zm = im->size_z;

	for (int z = 0; z < zm; z++) {
		for (int y = 0; y < ym; y++) {
			uint8_t *ptr = im->voxels + binary_offset(im, 0, y, z);
			uint8_t *pt0 = ptr;
			while (1) {
				/* Scan for next solid block.
				 * This will be the xa coordinate of a cube.
				 */
				ptr = memchr(ptr, 0xff, pt0 + xm - ptr);
				if (ptr == NULL) break; /* nothing found on this line ... */
				int startx = ptr - pt0;

				/* Scan for next free block. This will be the xb coordinate.
				 * What is found is the pos of the next free block
				 * so it's that of last free block + 1,
				 * which is exactly where our cube has to end
				 */
				ptr = memchr(ptr + 1, 0x00, pt0 + xm - ptr - 1);
				if (ptr == NULL) {
					/* No free block found. This means our
					 * row of solid blocks extends to the
					 * image boundary.
					 */
					cubedim(startx, y, z, xm, y+1, z+1, vnbuf, vertcount);
					break;
				} else {
					cubedim(startx, y, z, ptr - pt0, y+1, z+1, vnbuf, vertcount);
				}
			}
		}
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
			uint8_t *ptr = im->voxels + binary_offset(im, 0, y, z);
			uint8_t *pt0 = ptr;
			while (1) {
				ptr = memchr(ptr, 0xff, pt0 + xm - ptr);
				if (ptr == NULL) break;

				vertices += 6*2*3;

				ptr = memchr(ptr + 1, 0x00, pt0 + xm - ptr - 1);
				if (ptr == NULL) break;
			}
		}
	}

	return vertices;
}

