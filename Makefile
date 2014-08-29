COMMON_OPT = -fdiagnostics-color=auto -flto -Wall -Wextra -Os -ggdb -march=native -I.
CFLAGS = $(COMMON_OPT) -std=gnu99
CXXFLAGS = $(COMMON_OPT)


LDLIBS = -lofstd -ldcmdata -ldcmimage -ldcmimgle -ldcmjpeg \
	-lstdc++ -lm -lpotrace -lcl
LDFLAGS = -fopenmp -flto -Os -ggdb -L/usr/lib/beignet

_main: load_dcm.o polypartition.o trace.o \
image3d/binary.o image3d/grayscale.o image3d/packed.o image3d/bin_unpack.o image3d/i3d.o \
dilate_erode/packed.o dilate_erode/unpacked.o dilate_erode/reference.o \
floodfill/ff64.o floodfill/reference.o \
opencl/raycast.o opencl/clutil.o \
tri.o memandor.o makesolid.o wpnm/imdiff.o measure.o  _main.o

