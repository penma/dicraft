#include "tri.h"

#include <stdlib.h>

struct tris *tris_new() {
	struct tris *t = malloc(sizeof(struct tris));
	t->len = 0;
	t->tris = NULL;
	return t;
}

struct tri *tris_push(struct tris *tris) {
	tris->len++;
	tris->tris = realloc(tris->tris, sizeof(struct tri) * tris->len);
	return tris->tris + tris->len - 1;
}

