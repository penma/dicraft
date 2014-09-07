#define _GNU_SOURCE

#include "image3d/i3d.h"
#include "image3d/binary.h"
#include "image3d/grayscale.h"
#include "image3d/threshold.h"
#include "load_dcm.h"

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "measure.h"
#include "memandor.h"

#include "glv_image_processing.h"
#include "create_vertexlist.h"
#include "matrix.h"
#include "shaders.h"
#include "glsl_includes.h"

static void checkForOpenGLError(const char* file, int line) {
	GLenum glErr = glGetError();
	if (glErr != GL_NO_ERROR) {
		fprintf(stderr, "E: %s:%d: OpenGL error: %s\n", file, line, gluErrorString(glErr));
		exit(1);
	}
}
#define GL_ERROR() checkForOpenGLError(__FILE__, __LINE__)

static grayscale_t im_dicom = NULL;
static binary_t im_slots[2];
int image_generated = 0;

uint16_t tv = 1700;

static void dicom_load() {
	char *dicomdir = "dicom-files/01uk";
	im_dicom = load_dicom_dir(dicomdir);
}

static void rebinarize() {
	if (image_generated) {
		binary_free(im_slots[0]);
		binary_free(im_slots[1]);
	}

	im_slots[0] = binary_like(im_dicom);
	threshold(im_slots[0], im_dicom, tv);

	im_slots[1] = glvi_binarize(im_dicom, tv);

	image_generated = 1;
}

/* Rendering Binary image into VBO. */
static GLuint vao_id;
static GLuint vbo_id;
static int vbo_initialized = 0;
static int vbo_vertcount;

static int imseg_off[4];

static void dumpVoxels() {
	short *vnbuf = NULL;
	vbo_vertcount = 0;

	binary_t i_1_2 = binary_like(im_slots[0]);
	binary_t i_1_n2 = binary_like(im_slots[0]);
	binary_t i_n1_2 = binary_like(im_slots[0]);
	memand2(i_1_2->voxels, im_slots[0]->voxels, im_slots[1]->voxels, im_slots[0]->size_z * im_slots[0]->off_z);
	memandnot2(i_1_n2->voxels, im_slots[0]->voxels, im_slots[1]->voxels, im_slots[0]->size_z * im_slots[0]->off_z);
	memandnot2(i_n1_2->voxels, im_slots[1]->voxels, im_slots[0]->voxels, im_slots[0]->size_z * im_slots[0]->off_z);

	imseg_off[0] = 0;
	append_vertices_for_image(i_1_2, &vnbuf, &vbo_vertcount);
	imseg_off[1] = vbo_vertcount;
	append_vertices_for_image(i_1_n2, &vnbuf, &vbo_vertcount);
	imseg_off[2] = vbo_vertcount;
	append_vertices_for_image(i_n1_2, &vnbuf, &vbo_vertcount);
	imseg_off[3] = vbo_vertcount;

	glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
	glBufferData(GL_ARRAY_BUFFER, 2 * vbo_vertcount * 3 * sizeof(short), vnbuf, GL_STATIC_DRAW); GL_ERROR();
	glVertexAttribPointer(0, 3, GL_SHORT, GL_FALSE, 2*3*sizeof(short), 0); GL_ERROR();
	glVertexAttribPointer(1, 3, GL_SHORT, GL_FALSE, 2*3*sizeof(short), 3*sizeof(short)); GL_ERROR();
	glEnableVertexAttribArray(0); GL_ERROR();
	glEnableVertexAttribArray(1); GL_ERROR();

	memset(vnbuf, 0, 2 * vbo_vertcount * 3 * sizeof(short)); /* optional, bug check */
	free(vnbuf);

	fprintf(stderr, "%d vertices = %d floats = %ld byte\n",
		vbo_vertcount,
		2 * vbo_vertcount * 3,
		2 * vbo_vertcount * 3 * sizeof(short)
	);
}

static void update_vbo() {
	if (vbo_initialized) {
		glDeleteBuffers(1, &vbo_id);
	}

	glGenVertexArrays(1, &vao_id);
	glBindVertexArray(vao_id);

	glGenBuffers(1, &vbo_id);
	dumpVoxels();

	vbo_initialized = 1;
}

static void keyfun(GLFWwindow *win, int key, int scancode, int action, int mods) {
	(void)win; (void)scancode; (void)mods; /* unused */

	if (action != GLFW_PRESS && action != GLFW_REPEAT) {
		return;
	}

	int update_image = 0;

	if (key == GLFW_KEY_ESCAPE) {
		exit(0);
	} else if (key == GLFW_KEY_F2) {
		tv += 100;
		update_image = 1;
	} else if (key == GLFW_KEY_F1) {
		tv -= 100;
		update_image = 1;
	} else {
		fprintf(stderr, "key %d (sc %d)\n", key, scancode);
	}

	if (update_image) {
		fprintf(stderr, "Regenerating image with tv=%d\n", tv);
		measure_once("ReGen", "Binarize", { rebinarize(); });
		measure_once("ReGen", "3DRender", { update_vbo(); });
	}
}

static void resizefun(GLFWwindow *win, int width, int height) {
	(void)win; /* unused */

	fprintf(stderr, "%d x %d\n", width, height);
	glViewport(0, 0, width, height);
}

static uint64_t getus() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

int main() {
	GLFWwindow *window;

	if (!glfwInit()) {
		fprintf(stderr, "E: Failed to initialize GLFW ()\n");
		exit(1);
	}

	window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(1);
	}

	glfwSetKeyCallback(window, keyfun);
	glfwSetWindowSizeCallback(window, resizefun);

	glfwMakeContextCurrent(window);

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		exit(1);
	}

	dicom_load();
	rebinarize();
	update_vbo();

	GLuint shaderprog = make_shader_program(
		vertex_shader_glsl, vertex_shader_glsl_len,
		fragment_shader_glsl, fragment_shader_glsl_len
	);
	glUseProgram(shaderprog); GL_ERROR();

	mat4 mat_view, mat_proj;
	int angle = 0;
	v4sf cube_center = { im_slots[0]->size_x/2.0f, im_slots[0]->size_y/2.0f, im_slots[0]->size_z/2.0f, 0.0f };

	uint64_t t1, t2;
	while (!glfwWindowShouldClose(window)) {
		angle++;
		angle %= 360;

		/* Render here */
		t1 = getus();

		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		glClearColor(0.0f,0.0f,0.0f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glColor4f(1.0f, 0.5f, 0.2f, 1.0f);

		int width, height;
		glfwGetWindowSize(window, &width, &height);
		mat_perspective(40.0f / 180.f * M_PI, (float)width/height, 0.1f, 4000.0f, mat_proj);
		mat_lookat(
			(v4sf){ 500.f, 500.f, 200.f, 1.0f } * (v4sf){ cosf(angle / 180.0f * M_PI), sinf(angle / 180.0f * M_PI), 1.0f, 1.0f } + cube_center,
			cube_center,
			(v4sf){ 0.0f, 0.0f, 1.0f, 0.0f },
			mat_view
		);

		GLuint view_i = glGetUniformLocation(shaderprog, "mat_view"); GL_ERROR();
		glUniformMatrix4fv(view_i, 1, GL_FALSE, mat_view); GL_ERROR();
		GLuint proj_i = glGetUniformLocation(shaderprog, "mat_proj"); GL_ERROR();
		glUniformMatrix4fv(proj_i, 1, GL_FALSE, mat_proj); GL_ERROR();

		glUniform4f(glGetUniformLocation(shaderprog, "color"), 1.0f, 1.0f, 1.0f, 1.0f);
		glDrawArrays(GL_TRIANGLES, imseg_off[0], imseg_off[1] - imseg_off[0]);

		glUniform4f(glGetUniformLocation(shaderprog, "color"), 1.0f, 0.0f, 0.0f, 1.0f);
		glDrawArrays(GL_TRIANGLES, imseg_off[1], imseg_off[2] - imseg_off[1]);

		glUniform4f(glGetUniformLocation(shaderprog, "color"), 0.0f, 1.0f, 0.0f, 1.0f);
		glDrawArrays(GL_TRIANGLES, imseg_off[2], imseg_off[3] - imseg_off[2]);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		t2 = getus();
		fprintf(stderr, "took %06ldus = %10.5f FPS\r", t2 - t1, 1000000./(t2 - t1));

		/* Poll for and process events */
		glfwPollEvents();
	}
}
