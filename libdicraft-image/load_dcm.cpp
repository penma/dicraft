#include "image3d/grayscale.h"
#include "load_dcm.h"

#include "gdcmImageReader.h"

#include <stdint.h>
#include <dirent.h>
#include <unistd.h>

using namespace std;

static int load_single_image(const char *fn, grayscale_t target, const int z) {
	gdcm::ImageReader ir;
	ir.SetFileName(fn);
	ir.Read();
	const gdcm::Image &di = ir.GetImage();
	
	const unsigned int *dims = di.GetDimensions();
	int im_width = dims[0];
	int im_height = dims[1];

	if ((target->size_x != im_width) || (target->size_y != im_height)) {
		fprintf(stderr, "Image file dimensions (%dx%d) do not match that of the 3D image (%dx%d)\n",
			im_width, im_height,
			target->size_x, target->size_y);
		return 0;
	}

	if (z >= target->size_z) {
		fprintf(stderr, "Layer %d is outside bounds of target (size_z %d)\n",
			z, target->size_z);
		return 0;
	}

	di.GetBuffer((char*)(target->voxels + z * target->off_z));

	return 1;
}

static int dcm_dimensions(const char *fn, int *pwidth, int *pheight) {
	gdcm::ImageReader ir;
	ir.SetFileName(fn);
	ir.Read();
	const gdcm::Image &di = ir.GetImage();
	
	const unsigned int *dims = di.GetDimensions();
	*pwidth = dims[0];
	*pheight = dims[1];

	return 1;
}

grayscale_t load_dicom_dir(const char *dn) {
	int fnmaxlen = strlen(dn) + 20; /* should suffice.. */
	char fn[fnmaxlen];

	grayscale_t im = grayscale_new();

	/* find out number of layers (z size) */
	while (1) {
		snprintf(fn, fnmaxlen, "%s/3DSlice%d.dcm", dn, im->size_z + 1);

		if (access(fn, F_OK)) {
			/* file doesn't exist - so our currently found size is
			 * the z size
			 */
			break;
		} else {
			im->size_z++;
		}
	}

	/* find out x and y size */
	snprintf(fn, fnmaxlen, "%s/3DSlice1.dcm", dn);
	dcm_dimensions(fn, &im->size_x, &im->size_y);

	/* allocate all the memory! */
	grayscale_alloc(im);

	/* now we can load all images */
	for (int z = 0; z < im->size_z; z++) {
		snprintf(fn, fnmaxlen, "%s/3DSlice%d.dcm", dn, z+1);
		load_single_image(fn, im, z);
	}

	return im;
}

