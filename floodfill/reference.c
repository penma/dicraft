#include <stdio.h>
#include <stdlib.h>

#include "image3d/binary.h"

struct pt3d {
	int x, y, z;
};

struct pts3d {
	struct pt3d *a;
	int len;
};

static void pt_enqueue(struct pts3d *pts, binary_t im, int px, int py, int pz) {
	if (i3d_inside(im, px, py, pz)) {
		struct pt3d *p = pts->a + pts->len;
		if (pts->len >= im->size_x * im->size_y * im->size_z * 4) {
			printf("*** CRASH AHEAD *** pts->len %d >= %d (for %dx%dx%d)\n",
				pts->len,
				im->size_x * im->size_y * im->size_z * 4,
				im->size_x , im->size_y , im->size_z
			);
			exit(1);
		}

		p->x = px; p->y = py; p->z = pz;

		pts->len++;
	}
}

void floodfill_reference(binary_t dst, binary_t src, int xs, int ys, int zs) {
	uint8_t leColor = binary_at(src, xs, ys, zs);
	struct pts3d pts;
	pts.a = calloc(src->size_x * src->size_y * src->size_z * 4, sizeof(struct pt3d));
	pts.len = 0;

	pt_enqueue(&pts, src, xs, ys, zs);

	while (pts.len > 0) {
		pts.len--;
		struct pt3d *p = pts.a + pts.len;
		int px = p->x, py = p->y, pz = p->z;

		int off = binary_offset(src, px, py, pz);
		if (src->voxels[off] == leColor && dst->voxels[off] != leColor) {
			dst->voxels[off] = leColor;
			pt_enqueue(&pts, src, px - 1, py, pz);
			pt_enqueue(&pts, src, px + 1, py, pz);
			pt_enqueue(&pts, src, px, py - 1, pz);
			pt_enqueue(&pts, src, px, py + 1, pz);
			pt_enqueue(&pts, src, px, py, pz - 1);
			pt_enqueue(&pts, src, px, py, pz + 1);
		}
	}
	free(pts.a);
}

