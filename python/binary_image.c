#include <Python.h>

#include "binary_image.h"

#include "image3d/binary.h"
#include "wpnm/imdiff.h"
#include <string.h>

static PyObject *py_binary_image_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	py_binary_image *self = (py_binary_image *) type->tp_alloc(type, 0);

	if (self != NULL) {
		self->im = binary_new();
		if (self->im == NULL) {
			Py_DECREF(self);
			return NULL;
		}
	}

	return (PyObject *) self;
}

static int py_binary_image_init(py_binary_image *self, PyObject *args, PyObject *kwds) {
	PyObject *like = NULL;
	static char *kwlist[] = { "like", NULL };
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &like)) {
		return -1;
	}

	if (like != NULL) {
		if (! PyObject_TypeCheck(like, &py_type_binary_image)) {
			PyErr_SetString(PyExc_TypeError, "'like' needs a BinaryImage to copy the size from");
			return -1;
		}

		py_binary_image *like_bi = (py_binary_image *)like;

		self->im->size_x = like_bi->im->size_x;
		self->im->size_y = like_bi->im->size_y;
		self->im->size_z = like_bi->im->size_z;
		binary_alloc(self->im);
	}

	/* FIXME what if not? */

	return 0;
}

static PyObject *py_binary_image_copy(py_binary_image *self) {
	py_binary_image *copy = (py_binary_image *) py_type_binary_image.tp_alloc(&py_type_binary_image, 0);

	if (copy != NULL) {
		copy->im = binary_like(self->im);
		if (copy->im == NULL) {
			Py_DECREF(copy);
			return NULL;
		}

		memcpy(copy->im->voxels, self->im->voxels, self->im->size_z * self->im->off_z);
	}

	return (PyObject *) copy;
}

static PyObject *py_binary_image_write_pnm(py_binary_image *self, PyObject *args) {
	char *dn;

	if (!PyArg_ParseTuple(args, "s", &dn)) {
		return NULL;
	}

	write_difference(self->im, self->im, dn);

	Py_RETURN_NONE;
}

static void py_binary_image_dealloc(py_binary_image *self) {
	if (self->im != NULL) {
		binary_free(self->im);
	}

	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyMethodDef py_binary_image_methods[] = {
	{ "copy", (PyCFunction)py_binary_image_copy, METH_NOARGS, "New instance but same content" },
	{ "_write_pnm", (PyCFunction)py_binary_image_write_pnm, METH_VARARGS, "Write image to PNM files (test version)" },
	{NULL}
};

PyTypeObject py_type_binary_image = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "dicomprint.BinaryImage",
	.tp_basicsize = sizeof(py_binary_image),
	.tp_dealloc = (destructor)py_binary_image_dealloc,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_doc = "Binary 3D images",
	.tp_methods = py_binary_image_methods,
	.tp_init = (initproc)py_binary_image_init,
	.tp_new = py_binary_image_new,
};

