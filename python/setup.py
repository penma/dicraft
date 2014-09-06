from distutils.core import setup, Extension
setup(name="dicomprint", version="0.1", ext_modules=[
	Extension("dicomprint",
		sources = [ "module.c", "binary_image.c", "grayscale_image.c" ],
		extra_compile_args = [ "-std=gnu99", "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-strict-prototypes", "-Wno-declaration-after-statement" ],
		libraries = [ "dicomprint" ],
		include_dirs = [ "../libdicomprint" ],
		library_dirs = [ "../libdicomprint" ],
		runtime_library_dirs = [ "../libdicomprint" ]
	)
])
