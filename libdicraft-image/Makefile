include ../mf/common

CFLAGS = $(COMMON_CFLAGS) -fpic -I.
CXXFLAGS = $(COMMON_CXXFLAGS) -fpic -I. $(GDCM_INCLUDE)

LDLIBS_SO = -lgdcmCommon -lgdcmMSFF -lgdcmDSED -lstdc++ -lm

LIB_NAME = $(call libname,dicraft-image)
SO_NAME = $(call soname,dicraft-image,1)

$(LIB_NAME): load_dcm.o \
image3d/binary.o image3d/grayscale.o image3d/packed.o image3d/bin_unpack.o image3d/i3d.o image3d/threshold.o \
dilate_erode/packed.o dilate_erode/unpacked.o \
floodfill/ff64.o floodfill/isolate.o floodfill/ff2d.o \
memandor.o measure.o wpnm/imdiff.o

clean:
	find -name '*.o' -delete
	rm -f $(LIB_NAME) $(SO_NAME)
