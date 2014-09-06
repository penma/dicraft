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

typedef struct binary {
	enum i3d_type type; /* I3D_BINARY */
	int size_x, size_y, size_z;
	int off_y; /* y offset because of alignment */
	int off_z; /* off_y * size_y */
	uint8_t *restrict voxels;
} *binary_t;

binary_t binary_new();
void binary_alloc(binary_t);
void binary_free(binary_t);
binary_t _binary_like(struct i3d *im);
#define binary_like(im) _binary_like((struct i3d *)(im))

static inline int binary_offset(binary_t im, int x, int y, int z) {
	return x + y*im->off_y + z*im->off_z;
}

#define binary_at(im,x,y,z) ((im)->voxels[binary_offset((im),(x),(y),(z))])

#ifdef __cplusplus
#undef restrict
#endif

#ifdef __cplusplus
}
#endif

#endif
