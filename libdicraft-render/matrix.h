#ifndef MATRIX_H
#define MATRIX_H

typedef float v4sf __attribute__ ((__vector_size__ (16)));
typedef float mat4[16];

void mat_lookat(v4sf eye, v4sf obj, v4sf up, mat4 out);
void mat_perspective(float fovy, float aspect, float znear, float zfar, mat4 out);

#endif
