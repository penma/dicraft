#ifndef IMAGE3D_PACKED_H
#define IMAGE3D_PACKED_H

#include <stdint.h>

#include "image3d/i3d.h"

struct i3d_packed {
	enum i3d_type type; /* I3D_PACKED */
	int size_x, size_y, size_z;
	int off_y;
	int off_z; /* off_y * size_y */
	uint8_t *restrict voxels;

	/* actual pointer to free-able memory. */
	uint8_t *restrict _actual_voxels;
};

struct i3d_packed *i3d_packed_new();
void i3d_packed_alloc(struct i3d_packed *);
void i3d_packed_free(struct i3d_packed *);

#endif
