#ifndef IMAGE3D_PACKED_H
#define IMAGE3D_PACKED_H

#include <stdint.h>

#include "image3d/i3d.h"

typedef struct packed {
	enum i3d_type type; /* I3D_PACKED */
	int size_x, size_y, size_z;
	int off_y;
	int off_z; /* off_y * size_y */
	uint8_t *restrict voxels;

	/* actual pointer to free-able memory. */
	uint8_t *restrict _actual_voxels;
} *packed_t;

packed_t packed_new();
void packed_alloc(packed_t);
void packed_free(packed_t);
packed_t _packed_like(struct i3d *im);
#define packed_like(im) _packed_like((struct i3d *)(im))

#endif
