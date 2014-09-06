from distutils.core import setup, Extension
setup(name="dicomprint", version="0.1", ext_modules=[
	Extension("dicomprint",
		sources = [ "module.c", "binary_image.c", "grayscale_image.c" ],
		extra_compile_args = [ "-std=gnu99", "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-strict-prototypes" ],
		libraries = [ "dicomprint" ],
		include_dirs = [ "../c" ],
		library_dirs = [ "../c" ], runtime_library_dirs = [ "../c" ]
	)
])
