include ../mf/common

CFLAGS = $(COMMON_CFLAGS) -fpic -I. -I../libdicraft-image

LDLIBS_SO = -ldicraft-image $(OPENGL_LIBS) -lm
LDFLAGS_SO = -L../libdicraft-image

LIB_NAME = $(call libname,dicraft-render)
SO_NAME = $(call soname,dicraft-render,1)

$(LIB_NAME): create_vertexlist.o matrix.o shaders.o vertex_shader_inc.o fragment_shader_inc.o

%_inc.c: %.glsl
	xxd -i $^ $@

clean:
	find -name '*.o' -delete
	rm -f $(LIB_NAME) $(SO_NAME)


