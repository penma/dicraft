CFLAGS = -fdiagnostics-color=auto -fpic -Wall -Wextra -Os -ggdb -march=native -I. -I../libdicraft-image -std=gnu99

LDLIBS_SO = -ldicraft-image -lGLEW -lGL -lGLU -lm

SO_NAME = libdicraft-render.so
SO_VERSION = 1

$(SO_NAME): create_vertexlist.o matrix.o shaders.o vertex_shader_inc.o fragment_shader_inc.o
	$(CC) -shared -Wl,-soname,$(SO_NAME).$(SO_VERSION) -o $(SO_NAME).$(SO_VERSION)   $^ -L../libdicraft-image $(LDLIBS_SO)
	ln -fs $(SO_NAME).$(SO_VERSION) $(SO_NAME)

%_inc.c: %.glsl
	xxd -i $^ $@

clean:
	find -name '*_inc.c' -delete
	find -name '*.o' -delete
	rm -f $(SO_NAME) $(SO_NAME).$(SO_VERSION)

