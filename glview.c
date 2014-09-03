//#define FIXED_PL
//#define SIMPLE_CUBE

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

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>

#include "floodfill/ff64.h"
#include "floodfill/isolate.h"

#include "makesolid.h"
#include "measure.h"

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
	glShaderSource(shader, 1, &src, NULL); GL_ERROR();

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

typedef v4sf mat4[4];
static void m_mult(mat4 A, mat4 B, mat4 out) {
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			double v = 0.0f;
			for (int i = 0; i < 4; i++) {
				v += B[row][i] * A[i][col];
			}
			out[row][col] = v;
		}
	}
}

static void m_lookat(v4sf eye, v4sf obj, v4sf up, mat4 out) {
	v4sf f = v_normalize(obj - eye);
	v4sf s = v_normalize(v_cross(f, up));
	v4sf u = v_cross(s, f);

	memset(out, 0, sizeof(mat4));

	for (int i = 0; i < 3; i++) {
		out[i][0] = s[i];
		out[i][1] = u[i];
		out[i][2] = -f[i];
	}
	out[3][0] = -v_dot(s, eye);
	out[3][1] = -v_dot(u, eye);
	out[3][2] = v_dot(f, eye);
	out[3][3] = 1.0f;
}

static void m_perspective(float fovy, float aspect, float znear, float zfar, mat4 out) {
	float tanHalfFovy = tanf(fovy * 0.5f);
	memset(out, 0, sizeof(mat4));
	out[0][0] = 1.0f / (aspect * tanHalfFovy);
	out[1][1] = 1.0f / tanHalfFovy;
	out[2][2] = (znear + zfar) / (znear - zfar);
	out[2][3] = -1.0f;
	out[3][2] = (2.0f * znear * zfar) / (znear - zfar);
}

static void m_id4(mat4 out) {
	memset(out, 0, sizeof(mat4));
	for (int i = 0; i < 4; i++) {
		out[i][i] = 1.0f;
	}
}

static void m_transp(mat4 in, mat4 out) {
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			out[row][col] = in[col][row];
		}
	}
}

static void m_print(mat4 m, const char *n) {
	float *mm = m;
	fprintf(stderr, "++ %s ++\n", n);
	for (int r = 0; r < 4; r++) {
		fprintf(stderr, "[%d] %10.5f %10.5f %10.5f %10.5f\n", r, mm[4*r+0], mm[4*r+1], mm[4*r+2], mm[4*r+3]);
	}
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

		dilate_packed((struct i3d *)p0, (struct i3d *)p1);
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

		erode_packed((struct i3d *)p0, (struct i3d *)p1);
	}

	unpack_binary(im_out, p0);

	packed_free(p0);
	packed_free(p1);
}

static grayscale_t im_dicom = NULL;
static binary_t im_closed = NULL;

uint16_t tv = 1700;


static void dicom_load() {
	char *dicomdir = "/home/penma/dicom-foo/3DPrint_DICOM/001 Anatomie UK Patient 1/DVT UK Patient 1";
	im_dicom = load_dicom_dir(dicomdir);
}

static void rebinarize() {
	if (im_closed != NULL) {
		binary_free(im_closed);
	}

	#ifdef SIMPLE_CUBE
	im_closed = binary_like(im_dicom);return;
	#endif

	binary_t im_bin = binary_like(im_dicom);
	measure_once("Thres", "Iso", {
	threshold(im_bin, im_dicom, tv);

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
	im_closed = closed;
}

/* Rendering Binary image into VBO. */
GLuint vao_id;
GLuint vbo_id, vbo_id_n;
int vbo_initialized = 0;
int vbo_size;

static void cubedim(float xa, float ya, float za, float xb, float yb, float zb, float *obuf, float *nbuf) {
#   define V(a,b,c) obuf[0] = (a 1 > 0 ? xb : xa); obuf[1] = (b 1 > 0 ? yb : ya); obuf[2] = (c 1 > 0 ? zb : za); obuf += 3;
#   define N(a,b,c) for (int i = 0; i < 6; i++) { nbuf[0] = a; nbuf[1] = b; nbuf[2] = c; nbuf += 3; }
	N( 1.0, 0.0, 0.0); /* 1 2 4   2 3 4 */
		V(+,-,+); V(+,-,-); V(+,+,+);
		V(+,-,-); V(+,+,-); V(+,+,+);
	N( 0.0, 1.0, 0.0);
		V(+,+,+); V(+,+,-); V(-,+,+);
		V(+,+,-); V(-,+,-); V(-,+,+);
	N( 0.0, 0.0, 1.0);
		V(+,+,+); V(-,+,+); V(+,-,+);
		V(-,+,+); V(-,-,+); V(+,-,+);
	N(-1.0, 0.0, 0.0);
		V(-,-,+); V(-,+,+); V(-,-,-);
		V(-,+,+); V(-,+,-); V(-,-,-);
	N( 0.0,-1.0, 0.0);
		V(-,-,+); V(-,-,-); V(+,-,+);
		V(-,-,-); V(+,-,-); V(+,-,+);
	N( 0.0, 0.0,-1.0);
		V(-,-,-); V(-,+,-); V(+,-,-);
		V(-,+,-); V(+,+,-); V(+,-,-);
#undef V
#undef N
}

static void dumpVoxels() {
	float *vbuf = NULL, *nbuf = NULL;

	int pointsDrawn = 0;
	int cubesDrawn = 0;

	int startz = -1;

	int xm = im_closed->size_x, ym = im_closed->size_y, zm = im_closed->size_z;

	fprintf(stderr, "Rendering\n");

	#ifndef SIMPLE_CUBE
	for (int y = 0; y < ym; y++) {
	for (int x = 0; x < xm; x++) {
	for (int z = 0; z < zm; z++) {
		if (binary_at(im_closed, x, y, z)) {
			pointsDrawn++;
			if (startz == -1) {
				startz = z;
			}
		} else {
			if (startz != -1) {
				float xa = (float)x;
				float ya = (float)y;
				float za = (float)startz;
				float xb = (float)(x+1);
				float yb = (float)(y+1);
				float zb = (float)(z);

				vbuf = realloc(vbuf, (cubesDrawn + 1)*6*2*3*3*sizeof(float));
				nbuf = realloc(nbuf, (cubesDrawn + 1)*6*2*3*3*sizeof(float));
				cubedim(xa,ya,za,xb,yb,zb, vbuf + cubesDrawn*6*2*3*3, nbuf + cubesDrawn*6*2*3*3);
				cubesDrawn++;

				startz = -1;
			}
		}
	}

	if (startz != -1) {
		float xa = (float)x;
		float ya = (float)y;
		float za = (float)startz;
		float xb = (float)(x+1);
		float yb = (float)(y+1);
		float zb = (float)(zm);

		vbuf = realloc(vbuf, (cubesDrawn + 1)*6*2*3*3*sizeof(float));
		nbuf = realloc(nbuf, (cubesDrawn + 1)*6*2*3*3*sizeof(float));
		cubedim(xa,ya,za,xb,yb,zb, vbuf + cubesDrawn*6*2*3*3, nbuf + cubesDrawn*6*2*3*3);
		cubesDrawn++;
		startz = -1;
	}

	}
	}
	#else
		vbuf = realloc(vbuf, (cubesDrawn + 1)*6*2*3*3*sizeof(float));
		nbuf = realloc(nbuf, (cubesDrawn + 1)*6*2*3*3*sizeof(float));
		cubedim(0, 0, 0, xm, ym, zm, vbuf + cubesDrawn*6*2*3*3, nbuf + cubesDrawn*6*2*3*3);
		cubesDrawn++;
	#endif


	vbo_size = cubesDrawn * 6*2*3;

	glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vbo_size * 3, vbuf, GL_STATIC_DRAW); GL_ERROR();
	#ifdef SIMPLE_CUBE
	for (int i = 0; i < vbo_size; i++) {
		fprintf(stderr, "V[%d..] %f %f %f\n", i*3, vbuf[3*i], vbuf[3*i+1], vbuf[3*i+2]);
	}
	#endif
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL); GL_ERROR();
	glEnableVertexAttribArray(0); GL_ERROR();

	glBindBuffer(GL_ARRAY_BUFFER, vbo_id_n); GL_ERROR();
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vbo_size * 3, nbuf, GL_STATIC_DRAW); GL_ERROR();
	#ifdef SIMPLE_CUBE
	for (int i = 0; i < vbo_size; i++) {
		fprintf(stderr, "N[%d..] %f %f %f\n", i*3, nbuf[3*i], nbuf[3*i+1], nbuf[3*i+2]);
	}
	#endif
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL); GL_ERROR();
	glEnableVertexAttribArray(1); GL_ERROR();

	free(vbuf);

	fprintf(stderr, "Rendered %7d points in %7d cubes (%7.4f%%)\n", pointsDrawn, cubesDrawn, 100. * cubesDrawn / pointsDrawn);
}


static void update_vbo() {
	if (vbo_initialized) {
		glDeleteBuffers(1, &vbo_id);
		glDeleteBuffers(1, &vbo_id_n);
	}

	glGenVertexArrays(1, &vao_id);
	glBindVertexArray(vao_id);

	glGenBuffers(1, &vbo_id);
	glGenBuffers(1, &vbo_id_n);
	dumpVoxels();

	//glBindBuffer(GL_ARRAY_BUFFER, vbo_id); GL_ERROR();
	//glVertexPointer(3, GL_FLOAT, 0, NULL); GL_ERROR();
	//glBindBuffer(GL_ARRAY_BUFFER, vbo_id_n); GL_ERROR();
	//glNormalPointer(GL_FLOAT, 0, NULL); GL_ERROR();
	
	//glEnableClientState(GL_VERTEX_ARRAY); GL_ERROR();
	//glEnableClientState(GL_NORMAL_ARRAY); GL_ERROR();

	vbo_initialized = 1;
}

void do_light() {

	GLfloat light_position[] = { 3000.0, 1000.0, 3000.0, 0.0 };
	GLfloat light_ambient[]  = { 0.1, 0.1, 0.1, 1.0 };
	GLfloat light_diffuse[]  = { 0.8, 0.8, 0.8, 1.0 };
	GLfloat light_specular[] = { 0.8, 0.8, 0.8, 1.0 };
	glShadeModel (GL_FLAT);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glEnable(GL_COLOR_MATERIAL);
}

static void keyfun(GLFWwindow *win, int key, int scancode, int action, int mods) {
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
	fprintf(stderr, "%d x %d\n", width, height);
	glViewport(0, 0, width, height);
}

int main(int argc, char *argv[]) {
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

	#ifdef FIXED_PL
	do_light();
	#else
	GLuint shaderprog = make_shader_program("glview.vert", "glview.frag");
	glUseProgram(shaderprog); GL_ERROR();
	#endif

	mat4 m, v, p;
	m_id4(m);
	mat4 pv, mvp;
	int angle = 0;
	v4sf cube_center = { im_closed->size_x/2.0f, im_closed->size_y/2.0f, im_closed->size_z/2.0f, 0.0f };
	while (!glfwWindowShouldClose(window)) {
		angle++;
		angle %= 360;

		/* Render here */
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		glClearColor(0.0f,0.0f,0.0f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glColor4f(1.0f, 0.5f, 0.2f, 1.0f);

		int width, height;
		glfwGetWindowSize(window, &width, &height);
		m_perspective(40.0f / 180.f * M_PI, (float)width/height, 0.1f, 4000.0f, p);
		m_lookat(
			(v4sf){ 500.f, 500.f, 200.f, 1.0f } * (v4sf){ cosf(angle / 180.0f * M_PI), sinf(angle / 180.0f * M_PI), 1.0f, 1.0f } + cube_center,
			cube_center,
			(v4sf){ 0.0f, 0.0f, 1.0f, 0.0f },
			v
		);
		m_mult(p, v, pv);
		m_mult(pv, m, mvp);

		#ifdef FIXED_PL
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(p);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(v);
		#else
		GLuint view_i = glGetUniformLocation(shaderprog, "mat_view"); GL_ERROR();
		glUniformMatrix4fv(view_i, 1, GL_FALSE, v); GL_ERROR();
		GLuint proj_i = glGetUniformLocation(shaderprog, "mat_proj"); GL_ERROR();
		glUniformMatrix4fv(proj_i, 1, GL_FALSE, p); GL_ERROR();
		#endif

		glDrawArrays(GL_TRIANGLES, 0, vbo_size);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}
}
