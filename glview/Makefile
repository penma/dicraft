CFLAGS = -fdiagnostics-color=auto -flto -Wall -Wextra -Os -ggdb -march=native -I. -I../libdicomprint -I../libdpvoxelrender -std=gnu99

LDLIBS = -ldicomprint -ldpvoxelrender -lm -lGLEW -lGL -lGLU -lglfw
LDFLAGS = -L../libdpvoxelrender -L../libdicomprint

glview: glview.o glv_image_processing.o

clean:
	find -name '*.o' -delete
	rm -f glview

