COMMON_OPT = -fdiagnostics-color=auto -flto -fpic -Wall -Wextra -Os -ggdb -march=native -I.
CFLAGS = $(COMMON_OPT) -std=gnu99
CXXFLAGS = $(COMMON_OPT)

LDLIBS = -ldicomprint -lGL -lGLU -lpotrace
LDLIBS_SO = -lofstd -loflog -ldcmdata -ldcmimage -ldcmimgle -ldcmjpeg -lstdc++ -lm
LDFLAGS = -flto -Os -ggdb

SO_NAME = libdicomprint.so
SO_VERSION = 1

_main: load_dcm.o polypartition.o trace.o \
image3d/binary.o image3d/grayscale.o image3d/packed.o image3d/bin_unpack.o image3d/i3d.o image3d/threshold.o \
dilate_erode/packed.o dilate_erode/unpacked.o dilate_erode/reference.o \
floodfill/ff64.o floodfill/reference.o floodfill/isolate.o floodfill/ff2d.o \
tri.o memandor.o wpnm/imdiff.o measure.o  _main.o

glview: load_dcm.o \
image3d/binary.o image3d/grayscale.o image3d/packed.o image3d/bin_unpack.o image3d/i3d.o image3d/threshold.o \
dilate_erode/packed.o dilate_erode/unpacked.o \
floodfill/ff64.o floodfill/isolate.o floodfill/ff2d.o \
memandor.o measure.o glview.o

$(SO_NAME): load_dcm.o \
image3d/binary.o image3d/grayscale.o image3d/packed.o image3d/bin_unpack.o image3d/i3d.o image3d/threshold.o \
dilate_erode/packed.o dilate_erode/unpacked.o \
floodfill/ff64.o floodfill/isolate.o floodfill/ff2d.o \
memandor.o measure.o wpnm/imdiff.o
	$(CC) -shared -Wl,-soname,$(SO_NAME).$(SO_VERSION) -flto -o $(SO_NAME).$(SO_VERSION)   $^ $(LDLIBS_SO)
	ln -fs $(SO_NAME).$(SO_VERSION) $(SO_NAME)

clean-force:
	find -name '*.o' -delete
	rm -f $(SO_NAME) $(SO_NAME).$(SO_VERSION)

