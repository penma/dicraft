#include "image3d/packed.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct i3d_packed *i3d_packed_new() {
	struct i3d_packed *im = malloc(sizeof(struct i3d_packed));
	im->size_x = 0; im->size_y = 0; im->size_z = 0;
	im->off_y = -1; im->off_z = -1;
	im->type = I3D_PACKED;
	im->voxels = NULL;
	return im;
}

static inline int ceil_div(int num, int den) {
	return (num + den-1) / den;
}

static inline int to_mult(int v, int m) {
	return ceil_div(v, m) * m;
}


void i3d_packed_alloc(struct i3d_packed *im) {
	/* TODO: document the purpose etc of the padding */
	im->off_y = to_mult(ceil_div(im->size_x, 8), 4) + 4;
	im->off_z = im->size_y * im->off_y;
	im->_actual_voxels = calloc(im->size_z * im->off_z + 4, sizeof(uint8_t));
	im->voxels = im->_actual_voxels + 4;
}

void i3d_packed_free(struct i3d_packed *im) {
	free(im->_actual_voxels);
	free(im);
}

#include <stdio.h>

void i3d_packed_good(struct i3d_packed *im, char *name) {
	int found = 0;

	if (im->voxels - im->_actual_voxels != 4) {
		fprintf(stderr, "%p/%s E: voxels=%p actual_voxels=%p\n",
			im, name,
			im->voxels, im->_actual_voxels);
		return;
	}

	for (int i = 0; i < im->voxels - im->_actual_voxels; i++) {
		if (im->_actual_voxels[i] != 0x00) {
			fprintf(stderr, "%p/%s E: at x=%d: non-zero (%02x) byte before image start\n",
				im, name,
				(im->_actual_voxels - im->voxels) + i,
				im->_actual_voxels[i]);
			found = 1;
		}
	}

	for (int z = 0; z < im->size_z; z++) {
		for (int y = 0; y < im->size_y; y++) {
			uint8_t *plane = im->voxels + z*im->off_z + y*im->off_y;
			for (int x = ceil_div(im->size_x, 8); x < im->off_y; x++) {
				if (plane[x] != 0x00) {
					fprintf(stderr, "%p/%s E: z=%3d y=%3d at x=%d: non-zero (%02x) byte after line end (%d)\n",
							im, name, z, y, x, plane[x], ceil_div(im->size_x, 8));
					found = 1;
				}
			}
		}
	}
	if (!found) {
		fprintf(stderr, "%p/%s OK\n", im, name);
	}


}

