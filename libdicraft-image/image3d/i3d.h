#ifndef IMAGE3D_I3D_H
#define IMAGE3D_I3D_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum i3d_type { I3D_NONE = 0, I3D_BINARY, I3D_PACKED, I3D_GRAYSCALE };

struct i3d {
	enum i3d_type type;
	int size_x, size_y, size_z;
};

#define i3d_inside(im,x,y,z) ( (x) >= 0 && (y) >= 0 && (z) >= 0 && (x) < (im)->size_x && (y) < (im)->size_y && (z) < (im)->size_z )

void _i3d_free(struct i3d *);
#define i3d_free(im) _i3d_free((struct i3d *)(im))

#ifdef __cplusplus
}
#endif

#endif
