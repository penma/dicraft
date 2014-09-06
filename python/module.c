#include <Python.h>

#include "binary_image.h"
#include "grayscale_image.h"

#include "load_dcm.h"

static PyObject *py_load_dicom_dir(PyObject *self, PyObject *args) {
	char *dicom_dir;

	if (!PyArg_ParseTuple(args, "s", &dicom_dir)) {
		return NULL;
	}

	py_grayscale_image *py_im = (py_grayscale_image *) py_type_grayscale_image.tp_alloc(&py_type_grayscale_image, 0);

	if (py_im == NULL) {
		return NULL;
	}

	py_im->im = load_dicom_dir(dicom_dir);

	/* TODO error checking lol */

	return (PyObject *) py_im;
}

static PyMethodDef dicomprintmethods[] = {
	{ "load_dicom_dir", py_load_dicom_dir, METH_VARARGS, "Make an image from a directory full of dicom files" },
	{NULL}
};

static PyModuleDef dicomprintmodule = {
	PyModuleDef_HEAD_INIT,
	"dicomprint",
	"Converting Dicom files to something properly 3d printable",
	-1,
	dicomprintmethods, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC
PyInit_dicomprint(void) {
	py_type_binary_image.tp_new = PyType_GenericNew;
	if (PyType_Ready(&py_type_binary_image) < 0) {
		return NULL;
	}
	if (PyType_Ready(&py_type_grayscale_image) < 0) {
		return NULL;
	}

	PyObject *m = PyModule_Create(&dicomprintmodule);
	if (m == NULL) {
		return NULL;
	}

	Py_INCREF(&py_type_binary_image);
	PyModule_AddObject(m, "BinaryImage", (PyObject *)&py_type_binary_image);

	Py_INCREF(&py_type_grayscale_image);
	PyModule_AddObject(m, "GrayscaleImage", (PyObject *)&py_type_grayscale_image);

	return m;
}
