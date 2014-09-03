#define _GNU_SOURCE

#include "image3d/i3d.h"
#include "image3d/binary.h"
#include "image3d/grayscale.h"
#include "image3d/packed.h"
#include "image3d/bin_unpack.h"
#include "image3d/threshold.h"
#include "load_dcm.h"
#include "trace.h"
#include "tri.h"
#include "wpnm/imdiff.h"

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "floodfill/ff64.h"
#include "floodfill/isolate.h"

#include "makesolid.h"
#include "measure.h"

#include "opencl/raycast.h"

#define _ii3d_check_difference(a,b) do { \
	binary_t __ma_a = (a), __ma_b = (b); \
	fprintf(stderr, "Images %s and %s %s\n", \
		#a, \
		#b, \
		(memcmp(__ma_a->voxels, __ma_b->voxels, __ma_a->size_z * __ma_a->off_z) \
		? "differ" \
		: "are equal") \
	); \
} while (0)

#define extern_floodfill(name) extern void name(struct i3d *, struct i3d *, int, int, int);
#define extern_xform(name) extern void name(struct i3d *, struct i3d *);

extern_floodfill(floodfill_reference)
extern_xform(dilate_packed);
extern_xform( erode_packed);
extern_xform(dilate_unpacked);
extern_xform( erode_unpacked);
extern_xform(dilate_reference);
extern_xform( erode_reference);

static void dilate_times(binary_t im_out, binary_t im_in, int n) {
	packed_t p0, p1, swap_tmp;
	p0 = packed_like(im_in);
	p1 = packed_like(im_in);

	pack_binary(p0, im_in);

	for (int i = 0; i < n; i++) {
		swap_tmp = p0;
		p0 = p1;
		p1 = swap_tmp;

		dilate_packed((struct i3d *)p0, (struct i3d *)p1);
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

		erode_packed((struct i3d *)p0, (struct i3d *)p1);
	}

	unpack_binary(im_out, p0);

	packed_free(p0);
	packed_free(p1);
}

int main() {
	//char *dicomdir = "/home/penma/dicom-foo/3DPrint_DICOM/001 Anatomie UK Patient 1/DVT UK Patient 1";
	//uint16_t threshold = 1700;
	char *dicomdir = "/home/penma/dicom-foo/3DPrint_DICOM/003 Anatomie UK Patient 2/DVT UK Patient 2";
	uint16_t tv = 1700;

	grayscale_t im_dicom = load_dicom_dir(dicomdir);

	binary_t im_bin = binary_like(im_dicom);
	threshold(im_bin, im_dicom, tv);
	grayscale_free(im_dicom);

	isolate(im_bin, im_bin, im_bin->size_x/2, 120, 20);

	/* close along lines in xy */
	/*
	int close_lines[][4] = {
		{ 7, 274, 55, 324 },
		{ 393, 327, 447, 284 }
	};
	*/
	int close_lines[][4] = {
		{ 34, 322, 95, 357 },
		{ 376, 357, 430, 322 }
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


#if 0

#define DO_UNPACKED

#ifdef DO_REFERENCE
	struct i3d_binary *closed_reference = close_holes_with((struct close_holes_params){
		.image = im_bin,
		.algo_name = "Reference",
		ch_dilate_f(dilate_reference, 0),
		ch_erode_f(erode_reference, 0),
		.dilate_1 =  8, .erode_extra_1 = 2, .zmax1 = 110,
		.dilate_2 = 18, .erode_extra_2 = 1, .zmax2 = 85,
		ch_floodfill_f(floodfill_reference),
		.do_floodfill_dilated = 1
	});
	write_difference(im_bin, closed_reference, "Iclosed_ref/");
#endif

#ifdef DO_UNPACKED
	binary_t closed_up = close_holes_with((struct close_holes_params){
		.image = im_bin,
		.algo_name = "Unpacked (Ref)",
		ch_dilate_f(dilate_unpacked, 0),
		ch_erode_f(erode_unpacked, 0),
		.dilate_1 =  8, .erode_extra_1 = 2, .zmax1 = 110,
		.dilate_2 = 18, .erode_extra_2 = 1, .zmax2 = 85,
		ch_floodfill_f(floodfill_ff64),
		.do_floodfill_dilated = 1
	});
	write_difference(im_bin, closed_up, "Iclosed_up/");
#ifdef DO_REFERENCE
	_ii3d_check_difference(closed_reference, closed_up);
#endif
#endif

	binary_t closed_p = close_holes_with((struct close_holes_params){
		.image = im_bin,
		.algo_name = "Packed",
		ch_dilate_f(dilate_packed, 1),
		ch_erode_f(erode_packed, 1),
		.dilate_1 = 4, .erode_extra_1 = 2, .zmax1 = 120,
		.dilate_2 = 8, .erode_extra_2 = 2, .zmax2 = 90,
		ch_floodfill_f(floodfill_ff64),
		.do_floodfill_dilated = 0
	});

#if 0
	dilate_unpacked(tmp, closed_p);
	erode_unpacked(closed_p, tmp);

	memcpy(tmp->voxels, closed_p->voxels, im_bin->off_z * im_bin->size_z);
	memset(closed_p->voxels, 0xff, im_bin->off_z * im_bin->size_z);
	floodfill_ff64((struct i3d *)closed_p, (struct i3d *)tmp, 0, 0, 0);
#endif
	write_difference(im_bin, closed_p, "Iclosed_p/");

#ifdef DO_UNPACKED
	_ii3d_check_difference(closed_up, closed_p);
	write_difference(closed_p, closed_up, "Idiff/");
#endif

#endif
	binary_t insides = binary_like(im_bin);

	binary_t closed = binary_like(im_bin);
	memcpy(closed->voxels, im_bin->voxels, im_bin->off_z * im_bin->size_z);

	/* heavy hole-closing 0-109 */
	dilate_times(insides, im_bin, 10);
	remove_bubbles(insides, insides, 0, 0, 0);
	erode_times(insides, insides, 12);
	memor2(closed->voxels, im_bin->voxels, insides->voxels, im_bin->off_z * 110);

	/* weaker hole-closing 110-124 */
	dilate_times(insides, im_bin, 5);
	remove_bubbles(insides, insides, 0, 0, 0);
	erode_times(insides, insides, 7);
	memor2(closed->voxels + im_bin->off_z * 110, im_bin->voxels + im_bin->off_z * 110, insides->voxels + im_bin->off_z * 110, im_bin->off_z * (125 - 110));

	/* 2d-hole-closing 1-109 */
	binary_t _tm0 = binary_like(im_bin);
	for (int z = 1; z < 110; z++) {
		memcpy(_tm0->voxels + z * _tm0->off_z, closed->voxels + z * _tm0->off_z, _tm0->off_z);
		memset(closed->voxels + z * _tm0->off_z, 0xff, _tm0->off_z);
		floodfill2d(closed, _tm0, 0, 0, z);
	}
	binary_free(_tm0);

	remove_bubbles(closed, closed, 0, 0, 0);

	write_difference(im_bin, closed, "Iwat/");

	/*
	struct tris **tris = malloc(sizeof(struct tris *) * im_bin->size_z);
	#pragma omp parallel for
	for (int z = 0; z < im_bin->size_z; z++) {
		fprintf(stderr, "Tracing layers %d/%d\n", z, im_bin->size_z);
		tris[z] = tris_new();
		trace_layer(tris[z], im_bin, z);
	}
	fprintf(stderr, "Traced %d layers.\n", im_bin->size_z);

	for (int z = 0; z < im_bin->size_z; z++) {
		free(tris[z]->tris);
		free(tris[z]);
	}
	free(tris);
	*/

	binary_free(im_bin);
}
