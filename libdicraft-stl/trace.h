#ifndef TRACE_H
#define TRACE_H

#include "image3d/binary.h"
#include "tri.h"

#ifdef __cplusplus
extern "C" {
#endif

void trace_layer(struct tris *, binary_t, const int);

#ifdef __cplusplus
}
#endif

#endif
