#include <Python.h>

#include "grayscale_image.h"
#include "binary_image.h"

#include "image3d/grayscale.h"
#include "image3d/threshold.h"

static PyObject *py_grayscale_image_copy(py_grayscale_image *self) {
	py_grayscale_image *copy = (py_grayscale_image *) py_type_grayscale_image.tp_alloc(&py_type_grayscale_image, 0);

	if (copy != NULL) {
		copy->im = grayscale_new();
		if (copy->im == NULL) {
			Py_DECREF(copy);
			return NULL;
		}
		copy->im->size_x = self->im->size_x;
		copy->im->size_y = self->im->size_y;
		copy->im->size_z = self->im->size_z;
		grayscale_alloc(copy->im);

		memcpy(copy->im->voxels, self->im->voxels, self->im->size_z * self->im->off_z * sizeof(uint16_t));
	}

	return (PyObject *) copy;
}

static void py_grayscale_image_dealloc(py_grayscale_image *self) {
	if (self->im != NULL) {
		grayscale_free(self->im);
	}

	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *py_grayscale_image_threshold(py_grayscale_image *self, PyObject *args) {
	int threshold_value;

	if (!PyArg_ParseTuple(args, "i", &threshold_value)) {
		return NULL;
	}

	py_binary_image *py_im = (py_binary_image *) py_type_binary_image.tp_alloc(&py_type_binary_image, 0);

	if (py_im != NULL) {
		py_im->im = binary_like(self->im);
		if (py_im->im == NULL) {
			Py_DECREF(py_im);
			return NULL;
		}

		threshold(py_im->im, self->im, threshold_value);
	}

	return (PyObject *) py_im;
}

static PyMethodDef py_grayscale_image_methods[] = {
	{ "copy", (PyCFunction)py_grayscale_image_copy, METH_NOARGS, "New instance but same content" },
	{ "threshold", (PyCFunction)py_grayscale_image_threshold, METH_VARARGS, "Threshold to newly created binary image" },
	{NULL}
};

PyTypeObject py_type_grayscale_image = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"dicomprint.GrayscaleImage", /* tp_name */
	sizeof(py_grayscale_image), /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)py_grayscale_image_dealloc, /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Grayscale 3D images",     /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	py_grayscale_image_methods, /* tp_methods */
	0,                         /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	0,                         /* tp_init */
	PyType_GenericAlloc,       /* tp_alloc */
	PyType_GenericNew,         /* tp_new */
};

