#include "trace.h"

#include "tri.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <vector>

#include <potracelib.h>

#include "polypartition.h"

using namespace std;

static inline int mod(int a, int n) {
	return a>=n ? a%n : a>=0 ? a : n-1-(-1-a)%n;
}

static void pushpoint(vector<TPPLPoint> &pts, double x, double y) {
	TPPLPoint p;
	p.x = x;
	p.y = y;
	pts.push_back(p);
}

static int issufficientlyflat(potrace_dpoint_t cp[4]) {
	double ux = 3.0*cp[1].x - 2.0*cp[0].x - cp[3].x; ux *= ux;
	double uy = 3.0*cp[1].y - 2.0*cp[0].y - cp[3].y; uy *= uy;
	double vx = 3.0*cp[2].x - 2.0*cp[3].x - cp[0].x; vx *= vx;
	double vy = 3.0*cp[2].y - 2.0*cp[3].y - cp[0].y; vy *= vy;
	if (ux < vx) ux = vx;
	if (uy < vy) uy = vy;
	const double d_max = 0.25;
	return (ux+uy <= 16 * d_max * d_max);
}


static void midpoint(potrace_dpoint_t *pout, potrace_dpoint_t p1, potrace_dpoint_t p2) {
	pout->x = (p1.x + p2.x) / 2.0;
	pout->y = (p1.y + p2.y) / 2.0;
}

static void subdivide(potrace_dpoint_t c[4], potrace_dpoint_t l[4], potrace_dpoint_t r[4]) {
	l[0] = c[0];
	r[3] = c[3];
	potrace_dpoint_t m;
	midpoint(&m, c[1], c[2]);
	midpoint(&l[1], c[0], c[1]);
	midpoint(&r[2], c[2], c[3]);
	midpoint(&l[2], l[1], m);
	midpoint(&r[1], m, r[2]);
	midpoint(&r[0], l[2], r[1]); l[3] = r[0];
}

static void flattencurve(vector<TPPLPoint> &pts, potrace_dpoint_t cp[4]) {
	if (issufficientlyflat(cp)) {
		pushpoint(pts, cp[0].x, cp[0].y);
		/* straight segment a-d (excluding last vertex d) */
	} else {
		potrace_dpoint_t l[4], r[4];
		subdivide(cp, l, r);
		flattencurve(pts, l);
		flattencurve(pts, r);
	}
}

TPPLPoly curve2poly(potrace_curve_t *curve) {
	vector<TPPLPoint> pts;

	for (int i = 0; i < curve->n; i++) {
		potrace_dpoint_t *c = curve->c[i];
		potrace_dpoint_t *c1 = curve->c[mod(i-1,curve->n)];
		if (curve->tag[i] == POTRACE_CORNER) {
			pushpoint(pts, c1[2].x, c1[2].y);
			pushpoint(pts, c[1].x, c[1].y);
		} else if (curve->tag[i] == POTRACE_CURVETO) {
			potrace_dpoint_t cp[4];
			cp[0] = c1[2];
			cp[1] = c[0];
			cp[2] = c[1];
			cp[3] = c[2];
			flattencurve(pts, cp);
		} else {
			printf("i=%4d wat\n", i);
		}
	}

	TPPLPoly r;
	r.Init(pts.size());
	TPPLPoint *tpts = r.GetPoints();
	for (size_t i = 0; i < pts.size(); i++) {
		tpts[i].x = pts[i].x;
		tpts[i].y = pts[i].y;
	}

	if (r.GetOrientation() == TPPL_CW) {
		r.SetHole(true);
	}

	return r;
}

/*
void stlTri(
	ostream &fs, int nx, int ny, int nz,
	double x1, double y1, int z1,
	double x2, double y2, int z2,
	double x3, double y3, int z3
) {
	fs << "facet normal " << nx << " " << ny << " " << nz << endl << "outer loop" << endl;
	fs << "vertex " << x1*0.2 << " " << y1*0.2 << " " << z1*0.2 << endl;
	fs << "vertex " << x2*0.2 << " " << y2*0.2 << " " << z2*0.2 << endl;
	fs << "vertex " << x3*0.2 << " " << y3*0.2 << " " << z3*0.2 << endl;
	fs << "endloop" << endl << "endfacet" << endl;
}

void stlSide(
	ostream &fs,
	double x1, double y1, int z1,
	double x2, double y2, int z2
) {
	stlTri(fs, 0, 0, 0,
		x1, y1, z2,
		x1, y1, z1,
		x2, y2, z2);
	stlTri(fs, 0, 0, 0,
		x2, y2, z2,
		x1, y1, z1,
		x2, y2, z1);
}
*/

static void push_tri(struct tris *tris,
	double x1, double y1, int z1,
	double x2, double y2, int z2,
	double x3, double y3, int z3,
	int normal_z
) {
	struct tri *new_tri = tris_push(tris);
	new_tri->p[0].x = x1; new_tri->p[0].y = y1; new_tri->p[0].z = z1;
	new_tri->p[1].x = x2; new_tri->p[1].y = y2; new_tri->p[1].z = z2;
	new_tri->p[2].x = x3; new_tri->p[2].y = y3; new_tri->p[2].z = z3;

	if (normal_z) {
		new_tri->n.x = 0;
		new_tri->n.y = 0;
		new_tri->n.z = normal_z;
	} else {
		/* TODO calculate real normal */
		new_tri->n.x = new_tri->n.y = new_tri->n.z = 0.0;
	}
}

static void push_quad(struct tris *tris,
	double x1, double y1, int z1,
	double x2, double y2, int z2
) {
	push_tri(tris,
		x1, y1, z2,
		x1, y1, z1,
		x2, y2, z2,
		0);
	push_tri(tris,
		x2, y2, z2,
		x1, y1, z1,
		x2, y2, z1,
		0);
}

/* binary 3d image (slice)
 * ->
 * (traced vector graphic)
 * ->
 * (2d polygons for this layer)
 * ->
 * extruded 3d triangles
 *
 */
void trace_layer(struct tris *tris, binary_t src, const int z) {
	/* trace the slice with potrace */
	potrace_param_t *param = potrace_param_default();
	potrace_bitmap_t *bm = static_cast<potrace_bitmap_t*>(malloc(sizeof(potrace_bitmap_t)));

	bm->w = src->size_x;
	bm->h = src->size_y;
	int N = sizeof(potrace_word) * 8;
	bm->dy = (bm->w / N) + 1;

	bm->map = static_cast<potrace_word*>(calloc(bm->h * bm->dy, sizeof(potrace_word)));

	for (int y = 0; y < bm->h; y++) {
		for (int x = 0; x < bm->w; x++) {
			if (binary_at(src, x, y, z)) {
				bm->map[y * bm->dy + x/N] |= ((potrace_word)1 << (N - 1 - x%N));
			}
		}
	}

	potrace_state_t *res = potrace_trace(param, bm);

	/* we traced. generate polygons */
	list<TPPLPoly> polys;

	potrace_path_t *current = res->plist;

	while (current != NULL) {
		TPPLPoly poly = curve2poly(&current->curve);
		polys.push_back(poly);

		/* output the side faces */
		TPPLPoint *sides = poly.GetPoints();
		int nsides = poly.GetNumPoints();

		for (int i = 0; i < nsides; i++) {
			int i1 = mod(i-1, nsides);
			push_quad(tris,
				sides[i1].x, sides[i1].y, z,
				sides[i].x,  sides[i].y,  z+1
			);
		}

		current = current->next;
	}

	/* polygons added - partition into triangles */
	TPPLPartition part;
	list<TPPLPoly> part_tris;
	part.Triangulate_EC(&polys, &part_tris);

	for (list<TPPLPoly>::iterator iter = part_tris.begin(); iter != part_tris.end(); iter++) {
		TPPLPoint *tri = iter->GetPoints();

		/* top */
		push_tri(tris,
			tri[0].x, tri[0].y, z+1,
			tri[1].x, tri[1].y, z+1,
			tri[2].x, tri[2].y, z+1,
			+1);

		/* bottom */
		push_tri(tris,
			tri[0].x, tri[0].y, z,
			tri[2].x, tri[2].y, z,
			tri[1].x, tri[1].y, z,
			-1);
	}

	potrace_state_free(res);
	potrace_param_free(param);
	free(bm->map);
	free(bm);
}
