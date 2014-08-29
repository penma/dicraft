#ifndef IMDIFF_H
#define IMDIFF_H

#include "image3d/binary.h"

void write_difference(struct i3d_binary *old_im, struct i3d_binary *new_im, char *fnbase);

#endif

