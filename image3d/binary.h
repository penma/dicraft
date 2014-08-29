#ifndef IMAGE3D_BINARY_H
#define IMAGE3D_BINARY_H

#include <stdint.h>

#include "image3d/i3d.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
#define restrict __restrict__
#endif

struct i3d_binary {
	enum i3d_type type; /* I3D_BINARY */
	int size_x, size_y, size_z;
	int off_y; /* y offset because of alignment */
	int off_z; /* off_y * size_y */
	uint8_t *restrict voxels;
};

struct i3d_binary *i3d_binary_new();
void i3d_binary_alloc(struct i3d_binary *);
void i3d_binary_free(struct i3d_binary *);

static inline int i3d_binary_offset(struct i3d_binary *im, int x, int y, int z) {
	return x + y*im->off_y + z*im->off_z;
}

#define i3d_binary_at(im,x,y,z) ((im)->voxels[i3d_binary_offset((im),(x),(y),(z))])

#ifdef __cplusplus
#undef restrict
#endif

#ifdef __cplusplus
}
#endif

#endif
