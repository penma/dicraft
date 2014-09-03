/* needs at most: -lofstd -ldcm{data,image,imgle,jpeg} */

#include "image3d/grayscale.h"
#include "load_dcm.h"

#define HAVE_CONFIG_H
#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmimgle/dcmimage.h"
#include "dcmtk/dcmjpeg/djdecode.h"
#undef HAVE_CONFIG_H

#include <stdint.h>
#include <dirent.h>

using namespace std;

static int load_single_image(const char *fn, grayscale_t target, const int z) {
	/* ignored if called multiple times */
	DJDecoderRegistration::registerCodecs(EDC_photometricInterpretation);

	DcmFileFormat *dfile = new DcmFileFormat();
	OFCondition status = dfile->loadFile(fn);
	if (!status.good()) {
		fprintf(stderr, "Can't read DICOM file %s: %s\n", fn, status.text());
		return 0;
	}

	DcmDataset *dataset = dfile->getDataset();
	E_TransferSyntax xfer = dataset->getOriginalXfer();

	DicomImage *di = new DicomImage(dfile, xfer, 0 /* flags */, 0UL, 1UL);
	if (di == NULL) {
		/* OOM? */
		return 0;
	}

	if (di->getStatus() != EIS_Normal) {
		fprintf(stderr, "Error loading image from %s: %s\n", fn, DicomImage::getString(di->getStatus()));
		return 0;
	}

	if (!di->isMonochrome()) {
		fprintf(stderr, "Unexpected image: is not monochrome. Converting currently unsupported\n");
		return 0;
	}

	int im_width = di->getWidth();
	int im_height = di->getHeight();

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

	if (! di->getOutputData(
		target->voxels + z * target->off_z,
		target->off_z * sizeof(uint16_t),
		16, 0, 0
	)) {
		fprintf(stderr, "Error reading pixel data: %s\n", DicomImage::getString(di->getStatus()));
		return 0;
	}

	delete di;
	delete dfile;

	return 1;
}

static int dcm_dimensions(const char *fn, int *pwidth, int *pheight) {
	/* ignored if called multiple times */
	DJDecoderRegistration::registerCodecs(EDC_photometricInterpretation);

	DcmFileFormat *dfile = new DcmFileFormat();
	OFCondition status = dfile->loadFile(fn);
	if (!status.good()) {
		fprintf(stderr, "Can't read DICOM file %s: %s\n", fn, status.text());
		return 0;
	}

	DcmDataset *dataset = dfile->getDataset();
	E_TransferSyntax xfer = dataset->getOriginalXfer();

	DicomImage *di = new DicomImage(dfile, xfer, 0 /* flags */, 0UL, 1UL);
	if (di == NULL) {
		/* OOM? */
		return 0;
	}

	if (di->getStatus() != EIS_Normal) {
		fprintf(stderr, "Error opening image from %s: %s\n", fn, DicomImage::getString(di->getStatus()));
		return 0;
	}

	*pwidth = di->getWidth();
	*pheight = di->getHeight();

	delete di;
	delete dfile;

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
