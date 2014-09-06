#ifndef PY_BINARY_IMAGE_H
#define PY_BINARY_IMAGE_H

#include <Python.h>

#include "image3d/binary.h"

typedef struct {
	PyObject_HEAD
	binary_t im;
} py_binary_image;

extern PyTypeObject py_type_binary_image;

#endif
