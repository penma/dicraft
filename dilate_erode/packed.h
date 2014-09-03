#ifndef D_E_PACKED_H
#define D_E_PACKED_H

#include "image3d/packed.h"

void dilate_packed(restrict packed_t dst, restrict packed_t src);
void erode_packed(restrict packed_t dst, restrict packed_t src);

#endif
