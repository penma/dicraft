#ifndef LOAD_DCM_H
#define LOAD_DCM_H

#include "image3d/grayscale.h"

#ifdef __cplusplus
extern "C" {
#endif

grayscale_t load_dicom_dir(const char *);

#ifdef __cplusplus
}
#endif

#endif
