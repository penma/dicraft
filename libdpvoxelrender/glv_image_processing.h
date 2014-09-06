#ifndef GLV_IMAGE_PROCESSING_H
#define GLV_IMAGE_PROCESSING_H

#include "image3d/i3d.h"
#include "image3d/binary.h"
#include "image3d/grayscale.h"

#include <stdint.h>

binary_t glvi_binarize(grayscale_t im_dicom, uint16_t threshold_val);

#endif
