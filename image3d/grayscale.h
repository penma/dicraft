#ifndef IMAGE3D_GRAYSCALE_H
#define IMAGE3D_GRAYSCALE_H

#include <stdint.h>

#include "image3d/i3d.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
#define restrict __restrict__
#endif

struct i3d_grayscale {
	enum i3d_type type; /* I3D_GRAYSCALE */
	int size_x, size_y, size_z;
	int off_y; /* y offset because of alignment (or probably not) */
	int off_z; /* off_y * size_y */
	uint16_t *restrict voxels;
};

struct i3d_grayscale *i3d_grayscale_new();
void i3d_grayscale_alloc(struct i3d_grayscale *);
void i3d_grayscale_free(struct i3d_grayscale *);

static inline int i3d_grayscale_offset(struct i3d_grayscale *im, int x, int y, int z) {
	return x + y*im->off_y + z*im->off_z;
}

#define i3d_grayscale_at(im,x,y,z) ((im)->voxels[i3d_grayscale_offset((im),(x),(y),(z))])

#ifdef __cplusplus
#undef restrict
#endif

#ifdef __cplusplus
}
#endif

#endif
