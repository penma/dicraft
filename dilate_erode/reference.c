#include "image3d/binary.h"

/* dilate a single pixel.
 * this function can process any pixel, even those on the borders,
 * which requires it to check if it actually has neighbors,
 * and this makes it slow.
 */

static void dilate_single(struct i3d_binary *restrict dst, struct i3d_binary *restrict src, int x, int y, int z) {
	int set = 0;
	for (int zo = -1; zo <= +1; zo++) {
	for (int yo = -1; yo <= +1; yo++) {
	for (int xo = -1; xo <= +1; xo++) {
		if (i3d_inside(src, x+xo, y+yo, z+zo)) {
			if (i3d_binary_at(src, x+xo, y+yo, z+zo)) {
				set = 1;
				break;
			}
		}
	}}}
	i3d_binary_at(dst, x, y, z) = set ? 0xff : 0x00;
}

#define svb src->voxels

/* dilate whole image.
 * speed is improved by processing the non-border pixels on the xy planes first,
 * which allows skipping most of the coordinate bound checks.
 * afterwards they are filled in.
 */
void dilate_reference(struct i3d_binary *restrict dst, struct i3d_binary *restrict src) {
	int dy = src->off_y;
	int dz = src->off_z;
	for (int z = 0; z < src->size_z; z++) {
		for (int y = 1; y < src->size_y - 1; y++) {
			for (int x = 1; x < src->size_x - 1; x++) {
				int base = i3d_binary_offset(src, x, y, z-1);
				int set = 0;

				for (int zo = -1; zo <= +1; zo++) {
					if (z + zo >= 0 && z + zo < src->size_z) {
						if (
							svb[base-1-dy] || svb[base-dy] || svb[base+1-dy] ||
							svb[base-1   ] || svb[base   ] || svb[base+1]    ||
							svb[base-1+dy] || svb[base+dy] || svb[base+1+dy]
						) {
							set = 1;
						}
					}
					base += dz;
				}

				i3d_binary_at(dst, x, y, z) = set ? 0xff : 0x00;
			}
		}

		/* fill borders */
		for (int y = 0; y < src->size_y; y += src->size_y - 1) {
			for (int x = 0; x < src->size_x; x++) {
				dilate_single(dst, src, x, y, z);
			}
		}
		for (int x = 0; x < src->size_x; x += src->size_x - 1) {
			for (int y = 0; y < src->size_y; y++) {
				dilate_single(dst, src, x, y, z);
			}
		}
	}
}

/* erode whole image.
 *
 * there is no erode_single function here, because border pixels do not have
 * exactly 26 set neighbors, because they have less than 26 neighbors.
 * so they can always be set to 0.
 *
 * It would be needed for a variant where it requires all actual neighbors to
 * be 1. But that causes problems for our use case: if an image is dilated
 * many times so it connects to the image border, then subsequent erosion would
 * not remove the connection to the border.
 */
void erode_reference(struct i3d_binary *restrict dst, struct i3d_binary *restrict src) {
	int dy = src->off_y;
	int dz = src->off_z;
	for (int z = 0; z < src->size_z; z++) {
		for (int y = 1; y < src->size_y - 1; y++) {
			for (int x = 1; x < src->size_x - 1; x++) {
				int base = i3d_binary_offset(src, x, y, z-1);
				int set = 1;

				for (int zo = -1; zo <= +1; zo++) {
					if (z+zo < 0 || z+zo >= src->size_z || !(
						svb[base-1-dy] && svb[base-dy] && svb[base+1-dy] &&
						svb[base-1   ] && svb[base   ] && svb[base+1]    &&
						svb[base-1+dy] && svb[base+dy] && svb[base+1+dy]
					)) {
						set = 0;
					}
					base += dz;
				}

				i3d_binary_at(dst, x, y, z) = set ? 0xff : 0x00;
			}
		}
		/* the fugly borders */
		for (int y = 0; y < src->size_y; y += src->size_y - 1) {
			for (int x = 0; x < src->size_x; x++) {
				i3d_binary_at(dst, x, y, z) = 0;
			}
		}
		for (int x = 0; x < src->size_x; x += src->size_x - 1) {
			for (int y = 0; y < src->size_y; y++) {
				i3d_binary_at(dst, x, y, z) = 0;
			}
		}
	}
}

