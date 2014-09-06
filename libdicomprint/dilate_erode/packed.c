#include "image3d/packed.h"
#include "memandor.h"

#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h> /* ntohl/htonl */

static void memor_dilate(uint8_t *dst, uint8_t *src, int shift, int len) {
	memor2(dst, src, src + shift, shift);
	memor3(dst + shift, src, src + shift, src + 2*shift, len - 2*shift);
	memor2(dst + len-shift, src + len-2*shift, src + len-shift, shift);
}

static void memand_erode(uint8_t *dst, uint8_t *src, int shift, int len) {
	memset(dst, 0x00, shift);
	memand3(dst + shift, src, src + shift, src + 2*shift, len - 2*shift);
	memset(dst + len-shift, 0x00, shift);
}

static inline int ceil_div(int num, int den) {
	return (num + den-1) / den;
}

static inline int to_mult(int v, int m) {
	return ceil_div(v, m) * m;
}

static void dilate1bit(uint8_t *dst, uint8_t *src, size_t len) {
	uint32_t v0 = 0;
	uint32_t v1 = ntohl(*(uint32_t*)(src));
	uint32_t v2;
	for (int i = 0; i < len; i += 4) {
		v2 = ntohl(*(uint32_t*)(src + i + 4));
		uint32_t origv = v1;
		uint32_t rshiftv =
			(origv >> 1)
			|
			(v0 << 31)
		;
		uint32_t lshiftv =
			(origv << 1)
			|
			(v2 >> 31)
		;
		*(uint32_t*)(dst + i) = htonl(origv | rshiftv | lshiftv);

		v0 = v1;
		v1 = v2;
	}
}
static void erode1bit(uint8_t *dst, uint8_t *src, size_t len) {
	uint32_t v0 = 0;
	uint32_t v1 = ntohl(*(uint32_t*)(src));
	uint32_t v2;
	for (int i = 0; i < len; i += 4) {
		v2 = ntohl(*(uint32_t*)(src + i + 4));
		uint32_t origv = v1;
		uint32_t rshiftv =
			(origv >> 1)
			|
			(v0 << 31)
		;
		uint32_t lshiftv =
			(origv << 1)
			|
			(v2 >> 31)
		;
		*(uint32_t*)(dst + i) = htonl(origv & rshiftv & lshiftv);

		v0 = v1;
		v1 = v2;
	}
}

void dilate_packed(restrict packed_t dst, restrict packed_t src) {
	packed_t buf = packed_like(src);

	/* dst = dilate_x(src) */
	/* greatly abuses the padding before and after packed image data. */
	dilate1bit(
		dst->voxels,
		src->voxels,
		src->size_z * src->off_z - 4);

	/* buf = dilate_y(dst) */
	for (int z = 0; z < src->size_z; z++) {
		memor_dilate(
			buf->voxels + z*src->off_z,
			dst->voxels + z*src->off_z,
			src->off_y, src->off_z);
	}

	/* dst = dilate_z(buf) */
	memor_dilate(dst->voxels, buf->voxels, src->off_z, src->size_z * src->off_z);

	packed_free(buf);
}

void erode_packed(restrict packed_t dst, restrict packed_t src) {
	packed_t buf = packed_like(src);

	/* dst = erode_x(src) */
	/* greatly abuses the padding before and after packed image data. */
	erode1bit(
		dst->voxels,
		src->voxels,
		src->size_z * src->off_z - 4);

	/* buf = erode_y(dst) */
	for (int z = 0; z < src->size_z; z++) {
		memand_erode(
			buf->voxels + z*src->off_z,
			dst->voxels + z*src->off_z,
			src->off_y, src->off_z);
	}

	/* dst = erode_z(buf) */
	memand_erode(dst->voxels, buf->voxels, src->off_z, src->size_z * src->off_z);

	packed_free(buf);
}
