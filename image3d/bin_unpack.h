#ifndef IMAGE3D_BIN_UNPACK_H
#define IMAGE3D_BIN_UNPACK_H

#include "image3d/binary.h"
#include "image3d/packed.h"

void pack_binary(packed_t, binary_t);
void unpack_binary(binary_t, packed_t);

#endif
