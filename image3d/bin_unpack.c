#include "image3d/packed.h"
#include "image3d/binary.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if defined(__i386__) || defined(__x86_64__)
#define X86OID
#endif

/* TODO dieser Code benutzt an vielen Stellen noch xsize statt yoff... */

void pack_binary(restrict packed_t packed, restrict binary_t bin) {
	int xsize = packed->size_x, ysize = packed->size_y, zsize = packed->size_z;
	int b_yoff = bin->off_y, b_zoff = bin->off_z;
	int p_yoff = packed->off_y, p_zoff = packed->off_z;
	for (int z = 0; z < zsize; z++) {
		for (int y = 0; y < ysize; y++) {
			for (int px = 0; px < xsize/8; px++) {
				uint8_t *vbinp = bin->voxels + px*8 + y*b_yoff + z*b_zoff;
				uint64_t vbin = *(uint64_t*)vbinp;
				uint64_t bts = vbin & 0x0102040810204080U;
				uint32_t v32 = bts | (bts >> 32);
				uint16_t v16 = v32 | (v32 >> 16);
				uint8_t v8 = v16 | (v16 >> 8);
				packed->voxels[px + y*p_yoff + z*p_zoff] = v8;
			}
			memset(packed->voxels + (xsize/8) + y*p_yoff + z*p_zoff, 0x00, p_yoff - xsize/8);
			for (int x = (xsize/8) * 8; x < xsize; x++) {
				if (bin->voxels[x + y*b_yoff + z*b_zoff]) {
					packed->voxels[x/8 + y*p_yoff + z*p_zoff] |= (1 << (8 - 1 - x%8));
				}
			}
		}
	}
}

static void unpack_n(restrict binary_t bin, restrict packed_t packed) {
	int xsize = packed->size_x, ysize = packed->size_y, zsize = packed->size_z;
	int p_yoff = packed->off_y, p_zoff = packed->off_z;
	memset(bin->voxels, 0x00, zsize * bin->off_z);
	for (int z = 0; z < zsize; z++) {
		for (int y = 0; y < ysize; y++) {
			for (int x = 0; x < xsize; x++) {
				if (packed->voxels[x/8 + y*p_yoff + z*p_zoff] & (1 << (8 - 1 - x%8))) {
					binary_at(bin, x, y, z) = 0xff;
				}
			}
		}
	}
}

/* Optimized */

#ifdef X86OID

#include <mmintrin.h>

static void unpack_mmx(restrict binary_t bin, restrict packed_t packed) {
	int xsize = packed->size_x, ysize = packed->size_y, zsize = packed->size_z;
	int b_yoff = bin->off_y, b_zoff = bin->off_z;
	int p_yoff = packed->off_y, p_zoff = packed->off_z;
	__m64 mask = _mm_set_pi8(1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7);
	for (int z = 0; z < zsize; z++) {
		for (int y = 0; y < ysize; y++) {
			for (int px = 0; px < xsize/8; px++) {
				uint8_t vpack = packed->voxels[px + y*p_yoff + z*p_zoff];
				uint8_t *vbinp = bin->voxels + px*8 + y*b_yoff + z*b_zoff;
				/* 8x the packed value */
				__m64 mpack = _mm_set1_pi8(vpack);
				/* now each value corresponds to one bit */
				__m64 bts = _mm_and_si64(mpack, mask);
				/* where the bit is set (..and it matches the mask bit), write 0xff, otherwise 0x00 */
				/* comparison sets all bytes to 0xff where
				 * our masked value matches the mask.
				 * that is, where the pixel was set.
				 */
				*(__m64*)vbinp = _mm_cmpeq_pi8(bts, mask);
			}
			for (int x = (xsize/8) * 8; x < xsize; x++) {
				if (packed->voxels[x/8 + y*p_yoff + z*p_zoff] & (1 << (8 - 1 - x%8))) {
					bin->voxels[x + y*b_yoff + z*b_zoff] = 0xff;
				} else {
					bin->voxels[x + y*b_yoff + z*b_zoff] = 0x00;
				}
			}
		}
	}
}

#endif

static void (*resolve_unpack(void))(restrict binary_t, restrict packed_t) {
#ifdef X86OID
	__builtin_cpu_init();
	if (__builtin_cpu_supports("mmx")) {
		return unpack_mmx;
	} else {
		return unpack_n;
	}
#else
	return unpack_n;
#endif
}

void unpack_binary(restrict binary_t, restrict packed_t) __attribute__ ((ifunc("resolve_unpack")));

