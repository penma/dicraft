#include "matrix.h"

#include <math.h>
#include <string.h>

static float v_dot(v4sf a, v4sf b) {
	v4sf m = a * b;
	return m[0] + m[1] + m[2] + m[3];
}

static v4sf v_normalize(v4sf v) {
	float vsq = v_dot(v, v);
	return v / sqrtf(vsq);
}

static v4sf v_cross(v4sf a, v4sf b) {
	v4sf r = {
		a[1] * b[2] - b[1] * a[2],
		a[2] * b[0] - b[2] * a[0],
		a[0] * b[1] - b[0] * a[1],
		0.0f
	};
	return r;
}

void mat_lookat(v4sf eye, v4sf obj, v4sf up, mat4 out) {
	v4sf f = v_normalize(obj - eye);
	v4sf s = v_normalize(v_cross(f, up));
	v4sf u = v_cross(s, f);

	memset(out, 0, sizeof(mat4));

	for (int i = 0; i < 3; i++) {
		out[4*i + 0] = s[i];
		out[4*i + 1] = u[i];
		out[4*i + 2] = -f[i];
	}
	out[4*3 + 0] = -v_dot(s, eye);
	out[4*3 + 1] = -v_dot(u, eye);
	out[4*3 + 2] = v_dot(f, eye);
	out[4*3 + 3] = 1.0f;
}

void mat_perspective(float fovy, float aspect, float znear, float zfar, mat4 out) {
	float tanHalfFovy = tanf(fovy * 0.5f);
	memset(out, 0, sizeof(mat4));
	out[4*0 + 0] = 1.0f / (aspect * tanHalfFovy);
	out[4*1 + 1] = 1.0f / tanHalfFovy;
	out[4*2 + 2] = (znear + zfar) / (znear - zfar);
	out[4*2 + 3] = -1.0f;
	out[4*3 + 2] = (2.0f * znear * zfar) / (znear - zfar);
}

