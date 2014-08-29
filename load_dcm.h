#ifndef LOAD_DCM_H
#define LOAD_DCM_H

#include "image3d/grayscale.h"

#ifdef __cplusplus
extern "C" {
#endif

struct i3d_grayscale *load_dicom_dir(const char *);

#ifdef __cplusplus
}
#endif

#endif
