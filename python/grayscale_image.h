#ifndef PY_GRAYSCALE_IMAGE_H
#define PY_GRAYSCALE_IMAGE_H

#include <Python.h>

#include "image3d/grayscale.h"

typedef struct {
	PyObject_HEAD
	grayscale_t im;
} py_grayscale_image;

extern PyTypeObject py_type_grayscale_image;

#endif
