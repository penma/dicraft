from distutils.core import setup, Extension

# don't look at this.
# (it makes compiling on mingw possible, somehow)
import os
if os.name == 'nt':
	import distutils.sysconfig
	distutils.sysconfig.get_config_vars()
	distutils.sysconfig._config_vars["CC"] = "gcc"

setup(name="dicomprint", version="0.1", ext_modules=[
	Extension("dicomprint",
		sources = [ "module.c", "binary_image.c", "grayscale_image.c" ],
		extra_compile_args = [ "-include", "fixw64.h", "-std=gnu99", "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-strict-prototypes", "-Wno-declaration-after-statement" ],
		libraries = [ "dicraft-image" ],
		include_dirs = [ "../libdicraft-image" ],
		library_dirs = [ "../libdicraft-image", "." ],
		runtime_library_dirs = [ "../libdicraft-image" ]
	)
])

