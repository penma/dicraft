#ifndef IMAGE3D_BIN_UNPACK_H
#define IMAGE3D_BIN_UNPACK_H

#include "image3d/binary.h"
#include "image3d/packed.h"

void i3d_pack_binary(struct i3d_packed *, struct i3d_binary *);
void i3d_unpack_binary(struct i3d_binary *, struct i3d_packed *);

#endif
