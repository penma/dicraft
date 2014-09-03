#ifndef FLOODFILL_ISOLATE_H
#define FLOODFILL_ISOLATE_H

#include "image3d/binary.h"

void isolate(binary_t, binary_t, int, int, int);
void remove_bubbles(binary_t, binary_t, int, int, int);

#endif
