#ifndef MEMANDOR_H
#define MEMANDOR_H

#include <stdint.h>
#include <stddef.h>

extern void memand3(uint8_t *, uint8_t *, uint8_t *, uint8_t *, size_t);
extern void memor3(uint8_t *, uint8_t *, uint8_t *, uint8_t *, size_t);
extern void memor2(uint8_t *, uint8_t *, uint8_t *, size_t);
extern void memand2(uint8_t *, uint8_t *, uint8_t *, size_t);
extern void memandnot2(uint8_t *, uint8_t *, uint8_t *, size_t);

#endif
