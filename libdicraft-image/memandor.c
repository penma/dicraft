#include <stdint.h>
#include <stddef.h>

#define U8P uint8_t *restrict
#define U32P uint32_t *

/* Standard versions for any platform. */

static void memor3_n(U8P dst, U8P p1, U8P p2, U8P p3, size_t len) {
	uint32_t *d1 = (U32P)p1, *d2 = (U32P)p2, *d3 = (U32P)p3, *dd = (U32P)dst;
	for (size_t n = 0; n < (len/4 & ~0b11); n += 4) {
		dd[n+0] = d1[n+0] | d2[n+0] | d3[n+0];
		dd[n+1] = d1[n+1] | d2[n+1] | d3[n+1];
		dd[n+2] = d1[n+2] | d2[n+2] | d3[n+2];
		dd[n+3] = d1[n+3] | d2[n+3] | d3[n+3];
	}
	for (size_t n = len & ~(0b1111); n < len; n++) {
		dst[n] = p1[n] | p2[n] | p3[n];
	}
}

static void memand3_n(U8P dst, U8P p1, U8P p2, U8P p3, size_t len) {
	uint32_t *d1 = (U32P)p1, *d2 = (U32P)p2, *d3 = (U32P)p3, *dd = (U32P)dst;
	for (size_t n = 0; n < (len/4 & ~0b11); n += 4) {
		dd[n+0] = d1[n+0] & d2[n+0] & d3[n+0];
		dd[n+1] = d1[n+1] & d2[n+1] & d3[n+1];
		dd[n+2] = d1[n+2] & d2[n+2] & d3[n+2];
		dd[n+3] = d1[n+3] & d2[n+3] & d3[n+3];
	}
	for (size_t n = len & ~(0b1111); n < len; n++) {
		dst[n] = p1[n] & p2[n] & p3[n];
	}
}

void memor2(U8P dst, U8P p1, U8P p2, size_t len) {
	uint32_t *d1 = (U32P)p1, *d2 = (U32P)p2, *dd = (U32P)dst;
	for (size_t n = 0; n < (len/4 & ~0b11); n += 4) {
		dd[n+0] = d1[n+0] | d2[n+0];
		dd[n+1] = d1[n+1] | d2[n+1];
		dd[n+2] = d1[n+2] | d2[n+2];
		dd[n+3] = d1[n+3] | d2[n+3];
	}
	for (size_t n = len & ~(0b1111); n < len; n++) {
		dst[n] = p1[n] | p2[n];
	}
}

void memand2(U8P dst, U8P p1, U8P p2, size_t len) {
	uint32_t *d1 = (U32P)p1, *d2 = (U32P)p2, *dd = (U32P)dst;
	for (size_t n = 0; n < (len/4 & ~0b11); n += 4) {
		dd[n+0] = d1[n+0] & d2[n+0];
		dd[n+1] = d1[n+1] & d2[n+1];
		dd[n+2] = d1[n+2] & d2[n+2];
		dd[n+3] = d1[n+3] & d2[n+3];
	}
	for (size_t n = len & ~(0b1111); n < len; n++) {
		dst[n] = p1[n] & p2[n];
	}
}

void memandnot2(U8P dst, U8P p1, U8P p2, size_t len) {
	uint32_t *d1 = (U32P)p1, *d2 = (U32P)p2, *dd = (U32P)dst;
	for (size_t n = 0; n < (len/4 & ~0b11); n += 4) {
		dd[n+0] = d1[n+0] & ~d2[n+0];
		dd[n+1] = d1[n+1] & ~d2[n+1];
		dd[n+2] = d1[n+2] & ~d2[n+2];
		dd[n+3] = d1[n+3] & ~d2[n+3];
	}
	for (size_t n = len & ~(0b1111); n < len; n++) {
		dst[n] = p1[n] & ~p2[n];
	}
}

/* Slightly faster implementation with SSE2.
 * (~20%, tested on Ivy Bridge)
 */

#ifdef BUILD_SSE2

#include <emmintrin.h>
#define loadu_si128(p) _mm_loadu_si128((__m128i*)(p))
#define storeu_si128(p,b) _mm_storeu_si128((__m128i*)(p),(b))

static void memor3_sse2(U8P dst, U8P p1, U8P p2, U8P p3, size_t len) {
	for (size_t n = 0; n < (len & ~0b1111); n += 16) {
		__m128i v12 = _mm_or_si128(loadu_si128(p1 + n), loadu_si128(p2 + n));
		storeu_si128(dst + n, _mm_or_si128(v12, loadu_si128(p3 + n)));
	}
	for (size_t n = len & ~(0b1111); n < len; n++) {
		dst[n] = p1[n] | p2[n] | p3[n];
	}
}

static void memand3_sse2(U8P dst, U8P p1, U8P p2, U8P p3, size_t len) {
	for (size_t n = 0; n < (len & ~0b1111); n += 16) {
		__m128i v12 = _mm_and_si128(loadu_si128(p1 + n), loadu_si128(p2 + n));
		storeu_si128(dst + n, _mm_and_si128(v12, loadu_si128(p3 + n)));
	}
	for (size_t n = len & ~(0b1111); n < len; n++) {
		dst[n] = p1[n] & p2[n] & p3[n];
	}
}

#endif

#ifdef HAVE_IFUNC

/* Runtime CPU detection */

static void (*resolve_memor3(void))(U8P, U8P, U8P, U8P, size_t) {
	__builtin_cpu_init();
	if (__builtin_cpu_supports("sse2")) {
		return memor3_sse2;
	} else {
		return memor3_n;
	}
}

static void (*resolve_memand3(void))(U8P, U8P, U8P, U8P, size_t) {
	__builtin_cpu_init();
	if (__builtin_cpu_supports("sse2")) {
		return memand3_sse2;
	} else {
		return memand3_n;
	}
}

void memand3(U8P, U8P, U8P, U8P, size_t) __attribute__ ((ifunc("resolve_memand3")));
void memor3(U8P, U8P, U8P, U8P, size_t) __attribute__ ((ifunc("resolve_memor3")));

#else

/* No ifuncs, decide statically. */

void memand3(U8P dst, U8P p1, U8P p2, U8P p3, size_t len) {
#ifdef BUILD_SSE2
	memand3_sse2(dst, p1, p2, p3, len);
#else
	memand3_n(dst, p1, p2, p3, len);
#endif
}

void memor3(U8P dst, U8P p1, U8P p2, U8P p3, size_t len) {
#ifdef BUILD_SSE2
	memor3_sse2(dst, p1, p2, p3, len);
#else
	memor3_n(dst, p1, p2, p3, len);
#endif
}

#endif
