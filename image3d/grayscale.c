#include "image3d/grayscale.h"

#include <stdlib.h>
#include <string.h>

struct i3d_grayscale *i3d_grayscale_new() {
	struct i3d_grayscale *im = malloc(sizeof(struct i3d_grayscale));
	im->size_x = 0; im->size_y = 0; im->size_z = 0;
	im->off_y = -1; im->off_z = -1;
	im->type = I3D_GRAYSCALE;
	im->voxels = NULL;
	return im;
}

void i3d_grayscale_alloc(struct i3d_grayscale *im) {
	im->off_y = im->size_x;
	im->off_z = im->size_y * im->off_y;
	im->voxels = calloc(im->size_z * im->off_z, sizeof(uint16_t));
}

void i3d_grayscale_free(struct i3d_grayscale *im) {
	free(im->voxels);
	free(im);
}
