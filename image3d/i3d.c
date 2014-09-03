#include "image3d/i3d.h"
#include "image3d/binary.h"
#include "image3d/grayscale.h"
#include "image3d/packed.h"

void _i3d_free(struct i3d *im) {
	switch (im->type) {
	case I3D_BINARY:
		binary_free((binary_t)im);
		break;
	case I3D_GRAYSCALE:
		grayscale_free((grayscale_t)im);
		break;
	case I3D_PACKED:
		packed_free((packed_t)im);
		break;
	case I3D_NONE:
		/* wat */
		break;
	}
}
