#include "create_vertexlist.h"

#include "image3d/binary.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* Add a cube to given vertex+normal buffer, update the vertex count.
 * This function takes care of enlarging the buffer as needed.
 */
static void cubedim(short xa, short ya, short za, short xb, short yb, short zb, short **vnbuf, int *vertcount) {
	*vnbuf = realloc(*vnbuf, 2 * (*vertcount + 6*2*3) * 3 * sizeof(short));

	short *vn = *vnbuf + *vertcount * 2 * 3;

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
 * Updates the vertex count. Takes care of enlarging the buffer as needed.
 */
void append_vertices_for_image(binary_t im, short **vnbuf, int *vertcount) {
	/* for statistics */
	int pointsDrawn = 0;
	int cubesDrawn = 0;

	int startz = -1;
	int
		xm = im->size_x,
		ym = im->size_y,
		zm = im->size_z;

	for (int y = 0; y < ym; y++) {
		for (int x = 0; x < xm; x++) {
			for (int z = 0; z < zm; z++) {
				if (binary_at(im, x, y, z)) {
					pointsDrawn++;
					/* we're at the start, or in the middle of,
					 * a row of consecutive vertices.
					 */
					if (startz == -1) {
						startz = z;
					}
				} else if (startz != -1) {
					/* a row of consecutive vertices ends here.
					 * create a cube for them now
					 */
					cubedim(x, y, startz, x+1, y+1, z, vnbuf, vertcount);
					cubesDrawn++;
					startz = -1;
				}
			}

			/* if the row of vertices ends on the image border,
			 * we still need to create a cube
			 */
			if (startz != -1) {
				cubedim(x, y, startz, x+1, y+1, zm, vnbuf, vertcount);
				cubesDrawn++;
				startz = -1;
			}
		}
	}

	/* statistics */
	fprintf(stderr, "Rendered %7d points in %7d cubes (%7.4f%%)\n", pointsDrawn, cubesDrawn, 100. * cubesDrawn / pointsDrawn);
}

