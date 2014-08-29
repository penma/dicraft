#include "image3d/binary.h"
#include "memandor.h"

#include <stdlib.h>
#include <string.h>

static void memor_dilate(uint8_t *dst, uint8_t *src, int shift, int len) {
	for (int i = 0; i < shift; i++) {
		dst[i] = src[i] | src[i+shift];
	}
	memor3(dst + shift, src, src + shift, src + 2 * shift, len - 2 * shift);
	for (int i = len - shift; i < len; i++) {
		dst[i] = src[i-shift] | src[i];
	}
}

static void memand_erode(uint8_t *dst, uint8_t *src, int shift, int len) {
	memset(dst, 0x00, shift);
	memand3(dst + shift, src, src + shift, src + 2 * shift, len - 2 * shift);
	memset(dst + len - shift, 0x00, shift);
}

static struct i3d_binary *like(struct i3d_binary *orig) {
	struct i3d_binary *tcopy = i3d_binary_new();
	tcopy->size_x = orig->size_x;
	tcopy->size_y = orig->size_y;
	tcopy->size_z = orig->size_z;
	i3d_binary_alloc(tcopy);
	return tcopy;
}

void dilate_unpacked(struct i3d_binary *restrict dst, struct i3d_binary *restrict src) {
	struct i3d_binary *buf = like(src);

	/* dst = dilate_x(src) */
	for (int z = 0; z < src->size_z; z++) {
		for (int y = 0; y < src->size_y; y++) {
			memor_dilate(
				dst->voxels + z*src->off_z + y*src->off_y,
				src->voxels + z*src->off_z + y*src->off_y,
				1, src->size_x);
		}
	}

	/* buf = dilate_y(dst) */
	for (int z = 0; z < src->size_z; z++) {
		memor_dilate(
			buf->voxels + z*src->off_z,
			dst->voxels + z*src->off_z,
			src->off_y, src->off_z);
	}

	/* dst = dilate_z(buf) */
	memor_dilate(dst->voxels, buf->voxels, src->off_z, src->size_z * src->off_z);

	i3d_binary_free(buf);
}

void erode_unpacked(struct i3d_binary *restrict dst, struct i3d_binary *restrict src) {
	struct i3d_binary *buf = like(src);

	/* dst = erode_x(src) */
	for (int z = 0; z < src->size_z; z++) {
		for (int y = 0; y < src->size_y; y++) {
			memand_erode(
				dst->voxels + z*src->off_z + y*src->off_y,
				src->voxels + z*src->off_z + y*src->off_y,
				1, src->size_x);
		}
	}

	/* buf = erode_y(dst) */
	for (int z = 0; z < src->size_z; z++) {
		memand_erode(
			buf->voxels + z*src->off_z,
			dst->voxels + z*src->off_z,
			src->off_y, src->off_z);
	}

	/* dst = erode_z(buf) */
	memand_erode(dst->voxels, buf->voxels, src->off_z, src->size_z * src->off_z);

	i3d_binary_free(buf);
}

