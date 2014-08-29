#include "image3d/binary.h"

#include <stdlib.h>
#include <string.h>

struct i3d_binary *i3d_binary_new() {
	struct i3d_binary *im = malloc(sizeof(struct i3d_binary));
	im->size_x = 0; im->size_y = 0; im->size_z = 0;
	im->off_y = -1; im->off_z = -1;
	im->type = I3D_BINARY;
	im->voxels = NULL;
	return im;
}

void i3d_binary_alloc(struct i3d_binary *im) {
	im->off_y = (im->size_x + 7) / 8 * 8;
	im->off_z = im->size_y * im->off_y;
	im->voxels = calloc(im->size_z * im->off_z, sizeof(uint8_t));
}

void i3d_binary_free(struct i3d_binary *im) {
	free(im->voxels);
	free(im);
}
