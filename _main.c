#define _GNU_SOURCE

#include "image3d/i3d.h"
#include "image3d/binary.h"
#include "image3d/grayscale.h"
#include "image3d/packed.h"
#include "image3d/bin_unpack.h"
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

#include "makesolid.h"
#include "measure.h"

#include "opencl/raycast.h"

#define _ii3d_check_difference(a,b) do { \
	struct i3d_binary *__ma_a = (a), *__ma_b = (b); \
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

extern_floodfill(floodfill_ff64)
extern_floodfill(floodfill_reference)
extern_xform(dilate_packed);
extern_xform( erode_packed);
extern_xform(dilate_unpacked);
extern_xform( erode_unpacked);
extern_xform(dilate_reference);
extern_xform( erode_reference);

static struct i3d_packed *like_packed(struct i3d_binary *orig) {
	struct i3d_packed *tcopy = i3d_packed_new();
	tcopy->size_x = orig->size_x;
	tcopy->size_y = orig->size_y;
	tcopy->size_z = orig->size_z;
	i3d_packed_alloc(tcopy);
	return tcopy;
}

static void dilate_times(struct i3d_binary *im_out, struct i3d_binary *im_in, int n) {
	struct i3d_packed *p0, *p1, *swap_tmp;
	p0 = like_packed(im_in);
	p1 = like_packed(im_in);

	i3d_pack_binary(p0, im_in);

	for (int i = 0; i < n; i++) {
		swap_tmp = p0;
		p0 = p1;
		p1 = swap_tmp;

		dilate_packed((struct i3d *)p0, (struct i3d *)p1);
	}

	i3d_unpack_binary(im_out, p0);

	i3d_packed_free(p0);
	i3d_packed_free(p1);
}

int main() {

	//char *dicomdir = "/home/penma/dicom-foo/3DPrint_DICOM/001 Anatomie UK Patient 1/DVT UK Patient 1";
	//uint16_t threshold = 1700;
	char *dicomdir = "/home/penma/dicom-foo/3DPrint_DICOM/003 Anatomie UK Patient 2/DVT UK Patient 2";
	uint16_t threshold = 1700;

	fprintf(stderr, "Loading images from %s", dicomdir);
	struct i3d_grayscale *im_dicom = load_dicom_dir(dicomdir);
	fprintf(stderr, ".\n");

	/* threshold */
	struct i3d_binary *im_bin = i3d_binary_new();
	im_bin->size_x = im_dicom->size_x;
	im_bin->size_y = im_dicom->size_y;
	im_bin->size_z = im_dicom->size_z;
	i3d_binary_alloc(im_bin);

	fprintf(stderr, "Thresholding @ %d", threshold);
	//#pragma omp parallel for
	for (int z = 0; z < im_bin->size_z; z++) {
	for (int y = 0; y < im_bin->size_y; y++) {
	for (int x = 0; x < im_bin->size_x; x++) {
		int off = i3d_binary_offset(im_bin, x, y, z);
		if (im_dicom->voxels[x + y*im_dicom->off_y + z*im_dicom->off_z] >= threshold) {
			im_bin->voxels[off] = 0xff;
		} else {
			im_bin->voxels[off] = 0x00;
		}
	}}}
	fprintf(stderr, ".\n");

	i3d_grayscale_free(im_dicom);

	struct i3d_binary *tmp = i3d_binary_new();
	tmp->size_x = im_bin->size_x;
	tmp->size_y = im_bin->size_y;
	tmp->size_z = im_bin->size_z;
	i3d_binary_alloc(tmp);

	memcpy(tmp->voxels, im_bin->voxels, im_bin->off_z * im_bin->size_z);
	memset(im_bin->voxels, 0x00, im_bin->off_z * im_bin->size_z);
	floodfill_ff64((struct i3d *)im_bin, (struct i3d *)tmp, im_bin->size_x/2, 120, 20);

	/* close along lines in xy */
	#if 0
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
				if (i3d_binary_at(im_bin, c1x, c1y, z)) {
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
				if (i3d_binary_at(im_bin, c2x, c2y, z)) {
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
							i3d_binary_at(im_bin, ex+fx, ey+fy, z) = 0xff;
						}
					}
				}
			}
		}
	}
	#endif

	/*
	struct i3d_binary *d_inner = i3d_binary_new();
	d_inner->size_x = im_bin->size_x;
	d_inner->size_y = im_bin->size_y;
	d_inner->size_z = im_bin->size_z;
	i3d_binary_alloc(d_inner);

	struct i3d_binary *d_outer = i3d_binary_new();
	d_outer->size_x = im_bin->size_x;
	d_outer->size_y = im_bin->size_y;
	d_outer->size_z = im_bin->size_z;
	i3d_binary_alloc(d_outer);

	dilate_times(tmp, im_bin, 11);
	memset(d_inner->voxels, 0xff, im_bin->off_z * im_bin->size_z);
	floodfill_ff64((struct i3d *)d_inner, (struct i3d *)tmp, 0, 0, 0);

	dilate_times(d_outer, d_inner, 4);

	struct i3d_binary *d_check = i3d_binary_new();
	d_check->size_x = im_bin->size_x;
	d_check->size_y = im_bin->size_y;
	d_check->size_z = im_bin->size_z;
	i3d_binary_alloc(d_check);

	for (int z = 0; z < im_bin->size_z; z++) {
	for (int y = 0; y < im_bin->size_y; y++) {
	for (int x = 0; x < im_bin->size_x; x++) {
		uint8_t vinner = i3d_binary_at(d_inner, x, y, z);
		uint8_t vouter = i3d_binary_at(d_outer, x, y, z);
		uint8_t vimage = i3d_binary_at(im_bin, x, y, z);
		if (!vimage && vinner) {
			i3d_binary_at(d_check, x, y, z) = 0xff;
		}
	}}} */

	/*
	memcpy(tmp->voxels, im_rc->voxels, im_bin->off_z * im_bin->size_z);
	memset(im_rc->voxels, 0xff, im_bin->off_z * im_bin->size_z);
	floodfill_ff64((struct i3d *)im_rc, (struct i3d *)tmp, 0, 0, 0);
	*/
	//write_difference(im_bin, d_check, "Idiff/");
	//exit(0);

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
	struct i3d_binary *closed_up = close_holes_with((struct close_holes_params){
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

	struct i3d_binary *closed_p = close_holes_with((struct close_holes_params){
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

	write_difference(im_bin, im_bin, "Iwat/");

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

	i3d_binary_free(im_bin);
}
