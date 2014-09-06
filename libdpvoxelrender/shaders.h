#ifndef SHADERS_H
#define SHADERS_H

#include <GL/glew.h>

GLuint make_shader_program(
	const unsigned char *vert_src, const unsigned int vert_src_len,
	const unsigned char *frag_src, const unsigned int frag_src_len
);

#endif
