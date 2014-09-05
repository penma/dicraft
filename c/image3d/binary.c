#include "image3d/binary.h"

#include <stdlib.h>
#include <string.h>

binary_t binary_new() {
	binary_t im = malloc(sizeof(struct binary));
	im->size_x = 0; im->size_y = 0; im->size_z = 0;
	im->off_y = -1; im->off_z = -1;
	im->type = I3D_BINARY;
	im->voxels = NULL;
	return im;
}

void binary_alloc(binary_t im) {
	im->off_y = (im->size_x + 7) / 8 * 8;
	im->off_z = im->size_y * im->off_y;
	im->voxels = calloc(im->size_z * im->off_z, sizeof(uint8_t));
}

void binary_free(binary_t im) {
	free(im->voxels);
	free(im);
}

binary_t _binary_like(struct i3d *im) {
	binary_t bin = binary_new();
	bin->size_x = im->size_x;
	bin->size_y = im->size_y;
	bin->size_z = im->size_z;
	binary_alloc(bin);
	return bin;
}

