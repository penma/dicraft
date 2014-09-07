include ../mf/common

CFLAGS = $(COMMON_CFLAGS) -I. -I../libdicraft-image -I../libdicraft-render

LDLIBS = -ldicraft-image -ldicraft-render $(GLFW_LIBS) $(OPENGL_LIBS) -lm
LDFLAGS = $(LOCAL_LIB_PATHS) -L../libdicraft-image -L../libdicraft-render

glview: glview.o glv_image_processing.o

clean:
	find -name '*.o' -delete
	rm -f glview glview.exe
