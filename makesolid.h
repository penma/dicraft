#ifndef MAKESOLID_H
#define MAKESOLID_H

#include "image3d/i3d.h"
#include "image3d/binary.h"

typedef void (*imxform)(struct i3d *, struct i3d *);
typedef void (*floodfill_t)(struct i3d *, struct i3d *, int, int, int);

struct close_holes_params {
	struct i3d_binary *image;
	const char *algo_name;
	imxform dilate_f; int packed_dilate; const char *dilate_name; int dilate_1; int dilate_2;
	imxform erode_f ; int packed_erode;  const char *erode_name;  int erode_extra_1, erode_extra_2;
	int zmax1, zmax2;
	floodfill_t floodfill_f; const char *floodfill_name;

	int do_floodfill_dilated;
};

struct i3d_binary *close_holes_with(struct close_holes_params);

#define ch_floodfill_f(name) .floodfill_f = (floodfill_t)name, .floodfill_name = #name
#define ch_dilate_f(name,packed) .dilate_f = (imxform)name, .dilate_name = #name, .packed_dilate = packed
#define ch_erode_f(name,packed)  .erode_f  = (imxform)name, .erode_name  = #name, .packed_erode  = packed

#endif
