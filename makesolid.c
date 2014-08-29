#include "measure.h"
#include "image3d/i3d.h"
#include "image3d/binary.h"
#include "image3d/packed.h"
#include "image3d/bin_unpack.h"

#include <stdio.h>
#include <string.h>
#include "memandor.h"

#include "makesolid.h"

static struct i3d_binary *like_bin(struct i3d_binary *orig) {
	struct i3d_binary *tcopy = i3d_binary_new();
	tcopy->size_x = orig->size_x;
	tcopy->size_y = orig->size_y;
	tcopy->size_z = orig->size_z;
	i3d_binary_alloc(tcopy);
	return tcopy;
}

static struct i3d_packed *like_packed(struct i3d_packed *orig) {
	struct i3d_packed *tcopy = i3d_packed_new();
	tcopy->size_x = orig->size_x;
	tcopy->size_y = orig->size_y;
	tcopy->size_z = orig->size_z;
	i3d_packed_alloc(tcopy);
	return tcopy;
}

static void imcpy_bin(struct i3d_binary *restrict dst, struct i3d_binary *restrict src) {
	memcpy(dst->voxels, src->voxels, src->off_z * src->size_z);
}

static struct i3d_binary *dup_bin(struct i3d_binary *orig) {
	struct i3d_binary *tcopy = like_bin(orig);
	imcpy_bin(tcopy, orig);
	return tcopy;
}

static void swapim(struct i3d_binary *restrict dst, struct i3d_binary *restrict src) {
	uint8_t *tmp = src->voxels;
	src->voxels = dst->voxels;
	dst->voxels = tmp;
}
static void swapimp(struct i3d_packed *restrict dst, struct i3d_packed *restrict src) {
	uint8_t *tmp = src->voxels;
	src->voxels = dst->voxels;
	src->_actual_voxels = src->voxels - 4;
	dst->voxels = tmp;
	dst->_actual_voxels = tmp - 4;
}

#define call_dilate_f(out,in) params.dilate_f((struct i3d *)(out), (struct i3d *)(in))
#define call_erode_f(out,in) params.erode_f((struct i3d *)(out), (struct i3d *)(in))
#define call_floodfill_f(out,in,xs,ys,zs) params.floodfill_f((struct i3d *)(out), (struct i3d *)(in), (xs), (ys), (zs))

struct i3d_binary *close_holes_with(struct close_holes_params params) {
	struct i3d_binary *im = params.image;
	struct i3d_binary *tmp = like_bin(im);
	struct i3d_packed *ptmp = like_packed((struct i3d_packed *)im);

	measure_clear();

	start_measure("Whole", params.algo_name);

	/* Dilation. */
	struct i3d_binary *dilated1 = like_bin(im);
	struct i3d_binary *dilated2 = like_bin(im);
	if (params.packed_dilate) {
		struct i3d_packed *pout = like_packed(ptmp);
		measure_once("Pack","", {
			i3d_pack_binary(pout, im);
		});
		for (int i = 1; i <= params.dilate_1; i++) {
			fprintf(stderr, "Dilate[packed] %d/%d\r", i, params.dilate_2);
			measure_accum("Dilate", params.dilate_name, {
				call_dilate_f(ptmp, pout);
			});
			swapimp(ptmp, pout);
		}
		measure_once("Unpack","", {
			i3d_unpack_binary(dilated1, pout);
		});
		for (int i = params.dilate_1 + 1; i <= params.dilate_2; i++) {
			fprintf(stderr, "Dilate[packed] %d/%d\r", i, params.dilate_2);
			measure_accum("Dilate", params.dilate_name, {
				call_dilate_f(ptmp, pout);
			});
			swapimp(ptmp, pout);
		}
		measure_once("Unpack","", {
			i3d_unpack_binary(dilated2, pout);
		});
		i3d_packed_free(pout);
	} else {
		struct i3d_binary *out = dup_bin(im);
		for (int i = 1; i <= params.dilate_1; i++) {
			fprintf(stderr, "Dilate %d/%d\r", i, params.dilate_2);
			measure_accum("Dilate", params.dilate_name, {
				call_dilate_f(tmp, out);
			});
			swapim(tmp, out);
		}
		imcpy_bin(dilated1, out);
		for (int i = params.dilate_1 + 1; i <= params.dilate_2; i++) {
			fprintf(stderr, "Dilate %d/%d\r", i, params.dilate_2);
			measure_accum("Dilate", params.dilate_name, {
				call_dilate_f(tmp, out);
			});
			swapim(tmp, out);
		}
		imcpy_bin(dilated2, out);
		i3d_binary_free(out);
	}
	show_measure("Dilate", params.dilate_name);

	/* Floodfill. */
	if (params.do_floodfill_dilated) {
		memset(tmp->voxels, 0xff, im->size_z * im->off_z);
		measure_once("FFBackground1", params.floodfill_name, {
			call_floodfill_f(tmp, dilated1, 0, 0, 0);
		});
		swapim(tmp, dilated1);
		memset(tmp->voxels, 0xff, im->size_z * im->off_z);
		measure_once("FFBackground2", params.floodfill_name, {
			call_floodfill_f(tmp, dilated2, 0, 0, 0);
		});
		swapim(tmp, dilated2);
	}

	/* Erode. */
	struct i3d_binary *eroded1, *eroded2;
	if (params.packed_erode) {
		eroded1 = like_bin(im);
		eroded2 = like_bin(im);
		struct i3d_packed *pout = like_packed(ptmp);
		measure_once("Pack","", {
			i3d_pack_binary(pout, dilated1);
		});
		for (int i = 1; i <= params.dilate_1 + params.erode_extra_1; i++) {
			fprintf(stderr, "Erode[packed] %d/%d\r", i, params.dilate_1 + params.erode_extra_1);
			measure_accum("Erode1", params.erode_name, {
				call_erode_f(ptmp, pout);
			});
			swapimp(ptmp, pout);
		}
		measure_once("Unpack","", {
			i3d_unpack_binary(eroded1, pout);
		});
		measure_once("Pack","", {
			i3d_pack_binary(pout, dilated2);
		});
		for (int i = 1; i <= params.dilate_2 + params.erode_extra_2; i++) {
			fprintf(stderr, "Erode[packed] %d/%d\r", i, params.dilate_2 + params.erode_extra_2);
			measure_accum("Erode2", params.erode_name, {
				call_erode_f(ptmp, pout);
			});
			swapimp(ptmp, pout);
		}
		measure_once("Unpack","", {
			i3d_unpack_binary(eroded2, pout);
		});
		i3d_packed_free(pout);
	} else {
		eroded1 = dup_bin(dilated1);
		eroded2 = dup_bin(dilated2);
		for (int i = 1; i <= params.dilate_1 + params.erode_extra_1; i++) {
			fprintf(stderr, "Erode %d/%d\r", i, params.dilate_1 + params.erode_extra_1);
			measure_accum("Erode1", params.erode_name, {
				call_erode_f(tmp, eroded1);
			});
			swapim(tmp, eroded1);
		}
		for (int i = 1; i <= params.dilate_2 + params.erode_extra_2; i++) {
			fprintf(stderr, "Erode %d/%d\r", i, params.dilate_2 + params.erode_extra_2);
			measure_accum("Erode2", params.erode_name, {
				call_erode_f(tmp, eroded2);
			});
			swapim(tmp, eroded2);
		}
	}
	show_measure("Erode1", params.erode_name);
	show_measure("Erode2", params.erode_name);

	/* Merge original, eroded1 and eroded2. */
	struct i3d_binary *out = like_bin(im);
	memset(eroded1->voxels + params.zmax1 * im->off_z, 0x00, (im->size_z - params.zmax1) * im->off_z);
	memset(eroded2->voxels + params.zmax2 * im->off_z, 0x00, (im->size_z - params.zmax2) * im->off_z);
	memor2(tmp->voxels,  im->voxels, eroded1->voxels, im->size_z * im->off_z);
	memor2(out->voxels, tmp->voxels, eroded2->voxels, im->size_z * im->off_z);

	/* Floodfill so only the single large object remains. */
	memset(tmp->voxels, 0x00, im->size_z * im->off_z);
	measure_once("FFSolid", params.floodfill_name, {
		call_floodfill_f(tmp, out, out->size_x/2, 100, 20);
	});
	swapim(tmp, out);

	/* Clean up. */
	i3d_binary_free(tmp);
	i3d_packed_free(ptmp);
	i3d_binary_free(dilated1);
	i3d_binary_free(dilated2);
	i3d_binary_free(eroded1);
	i3d_binary_free(eroded2);

	show_measure("Dilate","X");
	show_measure("Dilate","Y");
	show_measure("Dilate","Z");
	show_measure("Erode","X");
	show_measure("Erode","Y");
	show_measure("Erode","Z");

	stop_show_measure("Whole", params.algo_name);

	fprintf(stderr, "\n* * * * * * * * * * * * * * * *\n");

	return out;
}

