#define _GNU_SOURCE

#include "image3d/binary.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

static void write_single(struct i3d_binary *old_im, struct i3d_binary *new_im, int z, char *fn) {
	int fd = open(fn, O_CREAT | O_WRONLY | O_TRUNC, 0666);
	if (fd < 0) {
		perror("write_difference: open");
		return;
	}

	FILE *fh = fdopen(fd, "w");

	fprintf(fh, "P6\n%d %d\n255\n", old_im->size_x, old_im->size_y);

	uint8_t
		c_ins[] = { 0x00, 0xff, 0x00 },
		c_del[] = { 0xff, 0x00, 0x00 },
		c_on[]  = { 0xff, 0xff, 0xff },
		c_off[] = { 0x00, 0x00, 0x00 };

	for (int y = 0; y < old_im->size_y; y++) {
		for (int x = 0; x < old_im->size_x; x++) {
			uint8_t vold = i3d_binary_at(old_im, x, y, z);
			uint8_t vnew = i3d_binary_at(new_im, x, y, z);
			uint8_t *to_write = vold
				? (vnew ? c_on  : c_del)
				: (vnew ? c_ins : c_off);
			fwrite(to_write, 3, 1, fh);
		}
	}

	fclose(fh);
}

void write_difference(struct i3d_binary *old_im, struct i3d_binary *new_im, char *fnbase) {
	for (int z = 0; z < old_im->size_z; z++) {
		char *fn;
		asprintf(&fn, "%s%03d.pnm", fnbase, z);
		write_single(old_im, new_im, z, fn);
		free(fn);
	}
}
