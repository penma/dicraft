#include "floodfill/isolate.h"
#include "floodfill/ff64.h"

#include "image3d/binary.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void iso_fill(binary_t dst, binary_t the_src, int x, int y, int z) {
	binary_t src;
	if (dst == the_src) {
		src = binary_like(the_src);
		memcpy(src->voxels, the_src->voxels, the_src->size_z * the_src->off_z);
	} else {
		src = the_src;
	}

	memset(dst->voxels, ~( binary_at(src, x, y, z) ), src->size_z * src->off_z);

	floodfill_ff64(dst, src, x, y, z);

	if (dst == the_src) {
		binary_free(src);
	}
}

void isolate(binary_t dst, binary_t src, int x, int y, int z) {
	if (binary_at(src, x, y, z) != 0xff) {
		fprintf(stderr, "E: Not isolating: There is no solid object at %d,%d,%d\n", x, y, z);
		return;
	}

	iso_fill(dst, src, x, y, z);
}

void remove_bubbles(binary_t dst, binary_t src, int x, int y, int z) {
	if (binary_at(src, x, y, z) != 0x00) {
		fprintf(stderr, "E: Not removing bubbles: There is no background at %d,%d,%d\n", x, y, z);
		return;
	}

	iso_fill(dst, src, x, y, z);
}

