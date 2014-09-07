CFLAGS = -fdiagnostics-color=auto -Wall -Wextra -Os -ggdb -march=native -I. -I../libdicraft-image -I../libdicraft-render -std=gnu99

LDLIBS = -ldicraft-image -ldicraft-render -lm -lGLEW -lGL -lGLU -lglfw
LDFLAGS = -L../libdicraft-image -L../libdicraft-render

glview: glview.o glv_image_processing.o

clean:
	find -name '*.o' -delete
	rm -f glview

