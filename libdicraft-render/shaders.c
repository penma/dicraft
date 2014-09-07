#include <stdlib.h>
#include <stdio.h>

#include "shaders.h"

#include <GL/glew.h>

static void checkForOpenGLError(const char* file, int line) {
	GLenum glErr = glGetError();
	if (glErr != GL_NO_ERROR) {
		fprintf(stderr, "E: %s:%d: OpenGL error: %s\n", file, line, gluErrorString(glErr));
		exit(1);
	}
}
#define GL_ERROR() checkForOpenGLError(__FILE__, __LINE__)

static GLuint compile_shader(const unsigned char *src, const unsigned int src_len, GLenum stype) {
	GLuint shader = glCreateShader(stype); GL_ERROR();
	glShaderSource(shader, 1, (const char *const *) &src, (const int *) &src_len); GL_ERROR();

	glCompileShader(shader); GL_ERROR();

	GLint err;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &err); GL_ERROR();
	if (!err) {
		fprintf(stderr, "Failure (%d) to compile\n", err);
		GLint loglen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &loglen); GL_ERROR();
		if (loglen > 0) {
			char *log = malloc(loglen);
			GLsizei written;
			glGetShaderInfoLog(shader, loglen, &written, log);
			fprintf(stderr, "[ log ]\n%s\n[ / log ]\n", log);
			free(log);
		}
		exit(1);
	}

	return shader;
}

GLuint make_shader_program(
	const unsigned char *vert_src, const unsigned int vert_src_len,
	const unsigned char *frag_src, const unsigned int frag_src_len
) {
	GLuint vert_o = compile_shader(vert_src, vert_src_len, GL_VERTEX_SHADER);
	GLuint frag_o = compile_shader(frag_src, frag_src_len, GL_FRAGMENT_SHADER);
	GLuint program = glCreateProgram(); GL_ERROR();

	if (!glIsProgram(program)) {
		fprintf(stderr, "Invalid program %d\n", program);
		exit(1);
	}
	if (!glIsShader(vert_o)) {
		fprintf(stderr, "Invalid vert_obj %d\n", vert_o);
		exit(1);
	}
	if (!glIsShader(frag_o)) {
		fprintf(stderr, "Invalid frag_obj %d\n", frag_o);
		exit(1);
	}

	glBindAttribLocation(program, 0, "VerPos"); GL_ERROR();
	glBindAttribLocation(program, 1, "VerNor"); GL_ERROR();

	glAttachShader(program, vert_o); GL_ERROR();
	glAttachShader(program, frag_o); GL_ERROR();
	glLinkProgram(program);

	GLint err;
	glGetProgramiv(program, GL_LINK_STATUS, &err);
	if (!err) {
		GLint loglen;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &loglen);
		if (loglen > 0) {
			char *log = malloc(loglen);
			GLsizei written;
			glGetProgramInfoLog(program, loglen, &written, log);
			fprintf(stderr, "[ log ]\n%s\n[ / log ]\n", log);
			free(log);
		}
		return 0;
	}

	return program;
}


