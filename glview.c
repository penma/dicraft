#define _GNU_SOURCE

#include "image3d/i3d.h"
#include "image3d/binary.h"
#include "image3d/grayscale.h"
#include "image3d/packed.h"
#include "image3d/bin_unpack.h"
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

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>

#include "floodfill/ff64.h"
#include "floodfill/ff2d.h"
#include "floodfill/isolate.h"

#include "dilate_erode/packed.h"

#include "makesolid.h"
#include "measure.h"
#include "memandor.h"

static void checkForOpenGLError(const char* file, int line) {
	GLenum glErr = glGetError();
	if (glErr != GL_NO_ERROR) {
		fprintf(stderr, "E: %s:%d: OpenGL error: %s\n", file, line, gluErrorString(glErr));
		exit(1);
	}
}
#define GL_ERROR() checkForOpenGLError(__FILE__, __LINE__)

static GLuint compile_shader(const char *fn, GLenum stype) {
	char *src = malloc(20000);
	FILE *fh = fopen(fn, "r");
	int srclen = fread(src, 1, 20000, fh);
	fclose(fh);

	src[srclen] = 0;

	GLuint shader = glCreateShader(stype); GL_ERROR();
	glShaderSource(shader, 1, (const char *const *) &src, NULL); GL_ERROR();

	free(src);

	glCompileShader(shader); GL_ERROR();

	GLint err;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &err); GL_ERROR();
	if (!err) {
		fprintf(stderr, "Failure (%d) to compile %s\n", err, fn);
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

static GLuint make_shader_program(const char *fnvert, const char *fnfrag) {
	GLuint vert_o = compile_shader(fnvert, GL_VERTEX_SHADER);
	GLuint frag_o = compile_shader(fnfrag, GL_FRAGMENT_SHADER);
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

/* matrix calculations. */
typedef float v4sf __attribute__ ((__vector_size__ (16)));

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

typedef float mat4[16];

static void m_lookat(v4sf eye, v4sf obj, v4sf up, mat4 out) {
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

static void m_perspective(float fovy, float aspect, float znear, float zfar, mat4 out) {
	float tanHalfFovy = tanf(fovy * 0.5f);
	memset(out, 0, sizeof(mat4));
	out[4*0 + 0] = 1.0f / (aspect * tanHalfFovy);
	out[4*1 + 1] = 1.0f / tanHalfFovy;
	out[4*2 + 2] = (znear + zfar) / (znear - zfar);
	out[4*2 + 3] = -1.0f;
	out[4*3 + 2] = (2.0f * znear * zfar) / (znear - zfar);
}

static void dilate_times(binary_t im_out, binary_t im_in, int n) {
	packed_t p0, p1, swap_tmp;
	p0 = packed_like(im_in);
	p1 = packed_like(im_in);

	pack_binary(p0, im_in);

	for (int i = 0; i < n; i++) {
		swap_tmp = p0;
		p0 = p1;
		p1 = swap_tmp;

		dilate_packed(p0, p1);
	}

	unpack_binary(im_out, p0);

	packed_free(p0);
	packed_free(p1);
}

static void erode_times(binary_t im_out, binary_t im_in, int n) {
	packed_t p0, p1, swap_tmp;
	p0 = packed_like(im_in);
	p1 = packed_like(im_in);

	pack_binary(p0, im_in);

	for (int i = 0; i < n; i++) {
		swap_tmp = p0;
		p0 = p1;
		p1 = swap_tmp;

		erode_packed(p0, p1);
	}

	unpack_binary(im_out, p0);

	packed_free(p0);
	packed_free(p1);
}

static grayscale_t im_dicom = NULL;
static binary_t im_slots[2];
int image_generated = 0;

uint16_t tv = 1700;


static void dicom_load() {
	char *dicomdir = "/home/penma/dicom-foo/3DPrint_DICOM/001 Anatomie UK Patient 1/DVT UK Patient 1";
	im_dicom = load_dicom_dir(dicomdir);
}

static void rebinarize() {
	if (image_generated) {
		for (int i = 0; i < 2; i++) {
			binary_free(im_slots[i]);
		}
	}

	binary_t im_bin = binary_like(im_dicom);
	measure_once("Thres", "Iso", {
	threshold(im_bin, im_dicom, tv);

	im_slots[0] = binary_like(im_bin);
	memcpy(im_slots[0]->voxels, im_bin->voxels, im_bin->size_z * im_bin->off_z);

	isolate(im_bin, im_bin, im_bin->size_x/2, 100, 20);
	});

	/* close along lines in xy */
	measure_once("Iso", "lines", {
	int close_lines[][4] = {
		{ 7, 274, 55, 324 },
		{ 393, 327, 447, 284 }
	};
	int num_close_lines = 2;

	for (int z = 0; z < im_bin->size_z; z++) {
		for (int i = 0; i < num_close_lines; i++) {
			int p1x = close_lines[i][0];
			int p1y = close_lines[i][1];
			int p2x = close_lines[i][2];
			int p2y = close_lines[i][3];
			int dx = p2x - p1x;
			int dy = p2y - p1y;
			int dp2 = 2.0f * sqrtf(dx*dx + dy*dy);

			int c1x, c1y, c2x, c2y;
			int found1 = 0, found2 = 0;

			/* move from p1 to p2 */
			for (int di = 0; di < dp2; di++) {
				c1x = p1x + dx*(float)di/dp2;
				c1y = p1y + dy*(float)di/dp2;
				if (binary_at(im_bin, c1x, c1y, z)) {
					c1x = p1x + dx * (float)(di + 3)/dp2;
					c1y = p1y + dy * (float)(di + 3)/dp2;
					found1 = 1;
					break;
				}
			}

			/* move from p2 to p1 */
			for (int di = 0; di < dp2; di++) {
				c2x = p2x - dx*(float)di/dp2;
				c2y = p2y - dy*(float)di/dp2;
				if (binary_at(im_bin, c2x, c2y, z)) {
					c2x = p2x - dx * (float)(di + 3)/dp2;
					c2y = p2y - dy * (float)(di + 3)/dp2;
					found2 = 1;
					break;
				}
			}

			if (found1 && found2) {
				/* draw line */
				int dcx = c2x - c1x;
				int dcy = c2y - c1y;
				for (int di = 0; di < dp2; di++) {
					int ex = c1x + dcx*(float)di/dp2;
					int ey = c1y + dcy*(float)di/dp2;

					for (int fy = -1; fy <= +1; fy++) {
						for (int fx = -1; fx <= +1; fx++) {
							binary_at(im_bin, ex+fx, ey+fy, z) = 0xff;
						}
					}
				}
			}
		}
	}
	});

	binary_t insides5  = binary_like(im_bin);
	binary_t insides10 = binary_like(im_bin);

	binary_t closed = binary_like(im_bin);
	memcpy(closed->voxels, im_bin->voxels, im_bin->off_z * im_bin->size_z);

	/* heavy hole-closing 0-109 */
	measure_once("Hole", "0-109", {
	dilate_times(insides5, im_bin, 5);
	dilate_times(insides10, insides5, 5);
	dilate_times(insides10, im_bin, 10);
	remove_bubbles(insides10, insides10, 0, 0, 0);
	erode_times(insides10, insides10, 12);
	memor2(closed->voxels, im_bin->voxels, insides10->voxels, im_bin->off_z * 110);
	});

	/* weaker hole-closing 110-124 */
	measure_once("Hole", "110-124", {
	remove_bubbles(insides5, insides5, 0, 0, 0);
	erode_times(insides5, insides5, 7);
	memor2(closed->voxels + im_bin->off_z * 110, im_bin->voxels + im_bin->off_z * 110, insides5->voxels + im_bin->off_z * 110, im_bin->off_z * (125 - 110));
	});

	/* 2d-hole-closing 1-109 */
	measure_once("Hole", "2D", {
	binary_t _tm0 = binary_like(im_bin);
	for (int z = 1; z < 110; z++) {
		memcpy(_tm0->voxels + z * _tm0->off_z, closed->voxels + z * _tm0->off_z, _tm0->off_z);
		memset(closed->voxels + z * _tm0->off_z, 0xff, _tm0->off_z);
		floodfill2d(closed, _tm0, 0, 0, z);
	}
	binary_free(_tm0);
	});

	measure_once("Hole", "Bubbles", {
	remove_bubbles(closed, closed, 0, 0, 0);
	});

	binary_free(insides5);
	binary_free(insides10);
	binary_free(im_bin);

	im_slots[1] = closed;
}

/* Rendering Binary image into VBO. */
GLuint vao_id;
GLuint vbo_id;
int vbo_initialized = 0;
int vbo_vertcount;

int imseg_off[4];

static void cubedim(short xa, short ya, short za, short xb, short yb, short zb, short **vnbuf, int *vertcount) {
	*vnbuf = realloc(*vnbuf, 2 * (*vertcount + 6*2*3) * 3 * sizeof(short));

	short *vn = *vnbuf + *vertcount * 2 * 3;

	short nx, ny, nz;
#   define V(a,b,c) \
	vn[0] = (a 1 > 0 ? xb : xa); \
	vn[1] = (b 1 > 0 ? yb : ya); \
	vn[2] = (c 1 > 0 ? zb : za); \
	vn[3] = nx; \
	vn[4] = ny; \
	vn[5] = nz; \
	vn += 6;
#   define N(a,b,c) nx = a; ny = b; nz = c;
	N( 1, 0, 0);
		V(+,-,+); V(+,-,-); V(+,+,+);
		V(+,-,-); V(+,+,-); V(+,+,+);
	N( 0, 1, 0);
		V(+,+,+); V(+,+,-); V(-,+,+);
		V(+,+,-); V(-,+,-); V(-,+,+);
	N( 0, 0, 1);
		V(+,+,+); V(-,+,+); V(+,-,+);
		V(-,+,+); V(-,-,+); V(+,-,+);
	N(-1, 0, 0);
		V(-,-,+); V(-,+,+); V(-,-,-);
		V(-,+,+); V(-,+,-); V(-,-,-);
	N( 0,-1, 0);
		V(-,-,+); V(-,-,-); V(+,-,+);
		V(-,-,-); V(+,-,-); V(+,-,+);
	N( 0, 0,-1);
		V(-,-,-); V(-,+,-); V(+,-,-);
		V(-,+,-); V(+,+,-); V(+,-,-);
#undef V
#undef N

	*vertcount += 6*2*3;
}

static void dump_im(binary_t im, short **vnbuf, int *vertcount) {
	int pointsDrawn = 0;
	int cubesDrawn = 0;

	int startz = -1;

	int
		xm = im->size_x,
		ym = im->size_y,
		zm = im->size_z;

	fprintf(stderr, "Rendering\n");

	for (int y = 0; y < ym; y++) {
	for (int x = 0; x < xm; x++) {
	for (int z = 0; z < zm; z++) {
		if (binary_at(im, x, y, z)) {
			pointsDrawn++;
			if (startz == -1) {
				startz = z;
			}
		} else {
			if (startz != -1) {
				cubedim(x, y, startz, x+1, y+1, z, vnbuf, vertcount);
				cubesDrawn++;

				startz = -1;
			}
		}
	}

	if (startz != -1) {
		cubedim(x, y, startz, x+1, y+1, zm, vnbuf, vertcount);
		cubesDrawn++;
		startz = -1;
	}

	}
	}


	fprintf(stderr, "Rendered %7d points in %7d cubes (%7.4f%%)\n", pointsDrawn, cubesDrawn, 100. * cubesDrawn / pointsDrawn);
}

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
	dump_im(i_1_2, &vnbuf, &vbo_vertcount);
	imseg_off[1] = vbo_vertcount;
	dump_im(i_1_n2, &vnbuf, &vbo_vertcount);
	imseg_off[2] = vbo_vertcount;
	dump_im(i_n1_2, &vnbuf, &vbo_vertcount);
	imseg_off[3] = vbo_vertcount;

	glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
	glBufferData(GL_ARRAY_BUFFER, 2 * vbo_vertcount * 3 * sizeof(short), vnbuf, GL_STATIC_DRAW); GL_ERROR();
	glVertexAttribPointer(0, 3, GL_SHORT, GL_FALSE, 2*3*sizeof(short), 0); GL_ERROR();
	glVertexAttribPointer(1, 3, GL_SHORT, GL_FALSE, 2*3*sizeof(short), 3*sizeof(short)); GL_ERROR();
	glEnableVertexAttribArray(0); GL_ERROR();
	glEnableVertexAttribArray(1); GL_ERROR();

	memset(vnbuf, 0, 2 * vbo_vertcount * 3 * sizeof(short)); /* optional, bug check */
	free(vnbuf);

	fprintf(stderr, "%d vertices = %d floats = %d byte\n",
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

	dicom_load();
	rebinarize();
	update_vbo();

	GLuint shaderprog = make_shader_program("glview.vert", "glview.frag");
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
		m_perspective(40.0f / 180.f * M_PI, (float)width/height, 0.1f, 4000.0f, mat_proj);
		m_lookat(
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
		fprintf(stderr, "took %06dus = %10.5f FPS\r", t2 - t1, 1000000./(t2 - t1));

		/* Poll for and process events */
		glfwPollEvents();
	}
}
