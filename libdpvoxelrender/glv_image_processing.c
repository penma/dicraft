#include "glv_image_processing.h"

#include "image3d/i3d.h"
#include "image3d/binary.h"
#include "image3d/grayscale.h"
#include "image3d/packed.h"
#include "image3d/bin_unpack.h"
#include "image3d/threshold.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "floodfill/ff64.h"
#include "floodfill/ff2d.h"
#include "floodfill/isolate.h"

#include "dilate_erode/packed.h"

#include "measure.h"
#include "memandor.h"

static void dilate_times(binary_t im_out, binary_t im_in, int n) {
	packed_t p0, p1, swap_tmp;
	p0 = packed_like(im_in);
	p1 = packed_like(im_in);

	pack_binary(p0, im_in);

	for (int i = 0; i < n; i++) {
		swap_tmp = p0;
		p0 = p1;
		p1 = swap_tmp;

		dilate_packed(p0, p1);
	}

	unpack_binary(im_out, p0);

	packed_free(p0);
	packed_free(p1);
}

static void erode_times(binary_t im_out, binary_t im_in, int n) {
	packed_t p0, p1, swap_tmp;
	p0 = packed_like(im_in);
	p1 = packed_like(im_in);

	pack_binary(p0, im_in);

	for (int i = 0; i < n; i++) {
		swap_tmp = p0;
		p0 = p1;
		p1 = swap_tmp;

		erode_packed(p0, p1);
	}

	unpack_binary(im_out, p0);

	packed_free(p0);
	packed_free(p1);
}

binary_t glvi_binarize(grayscale_t im_dicom, uint16_t threshold_val) {
	binary_t im_bin = binary_like(im_dicom);
	threshold(im_bin, im_dicom, threshold_val);

	isolate(im_bin, im_bin, im_bin->size_x/2, 100, 20);

	/* close along lines in xy */
	int close_lines[][4] = {
		{ 7, 274, 55, 324 },
		{ 393, 327, 447, 284 }
	};
	int num_close_lines = 2;

	for (int z = 0; z < im_bin->size_z; z++) {
		for (int i = 0; i < num_close_lines; i++) {
			int p1x = close_lines[i][0];
			int p1y = close_lines[i][1];
			int p2x = close_lines[i][2];
			int p2y = close_lines[i][3];
			int dx = p2x - p1x;
			int dy = p2y - p1y;
			int dp2 = 2.0f * sqrtf(dx*dx + dy*dy);

			int c1x, c1y, c2x, c2y;
			int found1 = 0, found2 = 0;

			/* move from p1 to p2 */
			for (int di = 0; di < dp2; di++) {
				c1x = p1x + dx*(float)di/dp2;
				c1y = p1y + dy*(float)di/dp2;
				if (binary_at(im_bin, c1x, c1y, z)) {
					c1x = p1x + dx * (float)(di + 3)/dp2;
					c1y = p1y + dy * (float)(di + 3)/dp2;
					found1 = 1;
					break;
				}
			}

			/* move from p2 to p1 */
			for (int di = 0; di < dp2; di++) {
				c2x = p2x - dx*(float)di/dp2;
				c2y = p2y - dy*(float)di/dp2;
				if (binary_at(im_bin, c2x, c2y, z)) {
					c2x = p2x - dx * (float)(di + 3)/dp2;
					c2y = p2y - dy * (float)(di + 3)/dp2;
					found2 = 1;
					break;
				}
			}

			if (found1 && found2) {
				/* draw line */
				int dcx = c2x - c1x;
				int dcy = c2y - c1y;
				for (int di = 0; di < dp2; di++) {
					int ex = c1x + dcx*(float)di/dp2;
					int ey = c1y + dcy*(float)di/dp2;

					for (int fy = -1; fy <= +1; fy++) {
						for (int fx = -1; fx <= +1; fx++) {
							binary_at(im_bin, ex+fx, ey+fy, z) = 0xff;
						}
					}
				}
			}
		}
	}

	binary_t insides5  = binary_like(im_bin);
	binary_t insides10 = binary_like(im_bin);

	binary_t closed = binary_like(im_bin);
	memcpy(closed->voxels, im_bin->voxels, im_bin->off_z * im_bin->size_z);

	/* heavy hole-closing 0-109 */
	measure_once("Hole", "0-109", {
	dilate_times(insides5, im_bin, 5);
	dilate_times(insides10, insides5, 5);
	dilate_times(insides10, im_bin, 10);
	remove_bubbles(insides10, insides10, 0, 0, 0);
	erode_times(insides10, insides10, 12);
	memor2(closed->voxels, im_bin->voxels, insides10->voxels, im_bin->off_z * 110);
	});

	/* weaker hole-closing 110-124 */
	measure_once("Hole", "110-124", {
	remove_bubbles(insides5, insides5, 0, 0, 0);
	erode_times(insides5, insides5, 7);
	memor2(closed->voxels + im_bin->off_z * 110, im_bin->voxels + im_bin->off_z * 110, insides5->voxels + im_bin->off_z * 110, im_bin->off_z * (125 - 110));
	});

	/* 2d-hole-closing 1-109 */
	measure_once("Hole", "2D", {
	binary_t _tm0 = binary_like(im_bin);
	for (int z = 1; z < 110; z++) {
		memcpy(_tm0->voxels + z * _tm0->off_z, closed->voxels + z * _tm0->off_z, _tm0->off_z);
		memset(closed->voxels + z * _tm0->off_z, 0xff, _tm0->off_z);
		floodfill2d(closed, _tm0, 0, 0, z);
	}
	binary_free(_tm0);
	});

	measure_once("Hole", "Bubbles", {
	remove_bubbles(closed, closed, 0, 0, 0);
	});

	binary_free(insides5);
	binary_free(insides10);
	binary_free(im_bin);

	return closed;
}

