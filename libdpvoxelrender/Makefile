CFLAGS = -fdiagnostics-color=auto -flto -fpic -Wall -Wextra -Os -ggdb -march=native -I. -I../libdicomprint -std=gnu99

LDLIBS_SO = -ldicomprint -lGLEW -lGL -lGLU -lm

SO_NAME = libdpvoxelrender.so
SO_VERSION = 1

$(SO_NAME): create_vertexlist.o matrix.o shaders.o vertex_shader_inc.o fragment_shader_inc.o
	$(CC) -shared -Wl,-soname,$(SO_NAME).$(SO_VERSION) -flto -o $(SO_NAME).$(SO_VERSION)   $^ -L../libdicomprint $(LDLIBS_SO)
	ln -fs $(SO_NAME).$(SO_VERSION) $(SO_NAME)

%_inc.c: %.glsl
	xxd -i $^ $@

clean:
	find -name '*_inc.c' -delete
	find -name '*.o' -delete
	rm -f $(SO_NAME) $(SO_NAME).$(SO_VERSION)

