COMMON_OPT = -fdiagnostics-color=auto -flto -fpic -Wall -Wextra -Os -ggdb -march=native -I.
CFLAGS = $(COMMON_OPT) -std=gnu99
CXXFLAGS = $(COMMON_OPT)

LDLIBS_SO = -lofstd -loflog -ldcmdata -ldcmimage -ldcmimgle -ldcmjpeg -lstdc++ -lm

SO_NAME = libdicomprint.so
SO_VERSION = 1

$(SO_NAME): load_dcm.o \
image3d/binary.o image3d/grayscale.o image3d/packed.o image3d/bin_unpack.o image3d/i3d.o image3d/threshold.o \
dilate_erode/packed.o dilate_erode/unpacked.o \
floodfill/ff64.o floodfill/isolate.o floodfill/ff2d.o \
memandor.o measure.o wpnm/imdiff.o
	$(CC) -shared -Wl,-soname,$(SO_NAME).$(SO_VERSION) -flto -o $(SO_NAME).$(SO_VERSION)   $^ $(LDLIBS_SO)
	ln -fs $(SO_NAME).$(SO_VERSION) $(SO_NAME)

clean:
	find -name '*.o' -delete
	rm -f $(SO_NAME) $(SO_NAME).$(SO_VERSION)

