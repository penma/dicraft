#include "image3d/grayscale.h"

#include <stdlib.h>
#include <string.h>

grayscale_t grayscale_new() {
	grayscale_t im = malloc(sizeof(struct grayscale));
	im->size_x = 0; im->size_y = 0; im->size_z = 0;
	im->off_y = -1; im->off_z = -1;
	im->type = I3D_GRAYSCALE;
	im->voxels = NULL;
	return im;
}

void grayscale_alloc(grayscale_t im) {
	im->off_y = im->size_x;
	im->off_z = im->size_y * im->off_y;
	im->voxels = calloc(im->size_z * im->off_z, sizeof(uint16_t));
}

void grayscale_free(grayscale_t im) {
	free(im->voxels);
	free(im);
}
