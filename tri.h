#ifndef TRI_H
#define TRI_H

#ifdef __cplusplus
extern "C" {
#endif

struct tri_point {
	double x, y, z;
};

struct tri {
	struct tri_point p[3];
	struct tri_point n;
};

struct tris {
	int len;
	struct tri *tris;
};

struct tris *tris_new();
struct tri *tris_push(struct tris *);

#ifdef __cplusplus
}
#endif

#endif
