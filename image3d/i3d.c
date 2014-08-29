#include "image3d/i3d.h"
#include "image3d/binary.h"
#include "image3d/grayscale.h"
#include "image3d/packed.h"

void _i3d_free(struct i3d *im) {
	switch (im->type) {
	case I3D_BINARY:
		i3d_binary_free((struct i3d_binary *)im);
		break;
	case I3D_CRAYSCALE:
		i3d_grayscale_free((struct i3d_grayscale *)im);
		break;
	case I3D_PACKED:
		i3d_packed_free((struct i3d_packed *)im);
		break;
	}
}
