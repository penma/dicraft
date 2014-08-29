#include <stdio.h>
#include <string.h>
#include <math.h>

#include <CL/cl.h>
#include "clutil.h"

#include "image3d/binary.h"
#include "measure.h"

#include <xmmintrin.h>

#define CHECK_ERR(s) do { \
	if (err != 0) { \
		fprintf(stderr, "E: %s:%d: %s: %d (%s)\n", \
			__FILE__, __LINE__, s, err, oclErrorString(err) \
		); \
		exit(1); \
	} \
} while (0)

static cl_program load_program(cl_context context, cl_device_id device_id, const char *fn) {
	cl_int err;

	char *cSourceCL = file_contents(fn);
	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&cSourceCL, NULL, &err);
	CHECK_ERR("clCreateProgramWithSource");

	clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

	cl_build_status build_status;
	err = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &build_status, NULL);
	CHECK_ERR("clGetProgramBuildInfo");

	char *build_log;
	size_t ret_val_size;
	err = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size);
	CHECK_ERR("clGetProgramBuildInfo");
	build_log = malloc(sizeof(char) * ret_val_size+1);
	err = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL);
	CHECK_ERR("clGetProgramBuildInfo");
	build_log[ret_val_size] = '\0';

	fprintf(stderr, "[ build log for %s ]\n%s\n[ end ]\n", fn, build_log);

	free(cSourceCL);

	return program;
}

struct i3d_binary *opencl_raycast(struct i3d_binary *im_in) {
	cl_int err;
	cl_event event;

	cl_context context = clCreateContextFromType(NULL, CL_DEVICE_TYPE_GPU, NULL, NULL, NULL);
	cl_device_id device_id;
	clGetDeviceIDs( NULL, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, NULL );
	cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, NULL);

	/* load the program */
	cl_program program = load_program(context, device_id, "opencl/raycast.cl");
	cl_kernel kernel2 = clCreateKernel(program, "part2", &err);
	CHECK_ERR("clCreateKernel");

	/* prepare input image */
	const cl_image_format format = {
		.image_channel_order = CL_RGBA,
		.image_channel_data_type = CL_UNORM_INT8
	};
	cl_image_desc desc = {
		.image_type = CL_MEM_OBJECT_IMAGE3D,
		.image_width = im_in->size_x,
		.image_height = im_in->size_y,
		.image_depth = im_in->size_z,
		.image_array_size = 0,
		.image_row_pitch = 0,
		.image_slice_pitch = 0,
		.num_mip_levels = 0, .num_samples = 0,
		.buffer = NULL
	};

	/* convert to RGBA.
	 * it seems CL_LUMINANCE or CL_INTENSITY are not supported everywhere.
	 */
	uint8_t *rgb_in;
	posix_memalign(&rgb_in, 8, im_in->size_x * im_in->size_y * im_in->size_z * 4);
	for (int z = 0; z < im_in->size_z; z++) {
		for (int y = 0; y < im_in->size_y; y++) {
			for (int x = 0; x < im_in->size_x; x++) {
				int i = x + y*im_in->size_x + z*im_in->size_y*im_in->size_x;
				rgb_in[i*4] = rgb_in[i*4 + 1] = rgb_in[i*4 + 2] =
					i3d_binary_at(im_in, x, y, z);
				rgb_in[i*4 + 3] = 0xff;
			}
		}
	}
	cl_mem input_image = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, &format, &desc, rgb_in, &err);
	CHECK_ERR("clCreateImage");

	/* prepare an output image */
	uint8_t *rgb_out = malloc(im_in->size_x * im_in->size_y * im_in->size_z * 4);
	memset(rgb_out, 0xff, im_in->size_x * im_in->size_y * im_in->size_z * 4);
	cl_mem output_image = clCreateImage(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, &format, &desc, rgb_out, &err);
	CHECK_ERR("clCreateImage");

	/* create some kernels */
	const int planekernels_n = 10;
	cl_kernel planekernels[planekernels_n];
	for (int i = 0; i < planekernels_n; i++) {
		planekernels[i] = clCreateKernel(program, "plane", &err);
		CHECK_ERR("clCreateKernel");

		err = clSetKernelArg(planekernels[i], 0, sizeof(cl_mem), &input_image);
		CHECK_ERR("clSetKernelArg");
		err = clSetKernelArg(planekernels[i], 1, sizeof(cl_mem), &output_image);
		CHECK_ERR("clSetKernelArg");

		float angle = M_PI * 2.0f * (float)i / planekernels_n;
		cl_float4 ray_dir = { cosf(angle), sinf(angle), 0.0f, 0.0f };
		cl_float4 v0 = { -ray_dir.y, ray_dir.x, 0.0f, 0.0f };
		cl_float4 v1 = { 0.0f, 0.0f, 1.0f, 0.0f };
		cl_float max_ray_len = sqrtf(im_in->size_x*im_in->size_x + im_in->size_y*im_in->size_y);
		cl_float4 origin = {
			im_in->size_x*0.5f - ray_dir.x*max_ray_len*0.5f - v0.x*max_ray_len*0.5f,
			im_in->size_y*0.5f - ray_dir.y*max_ray_len*0.5f - v0.y*max_ray_len*0.5f,
			0.0f,
			0.0f
		};

		fprintf(stderr, "Origin: %f %f %f\n", origin.x, origin.y, origin.z);
		fprintf(stderr, "v0: %f %f %f\n", v0.x, v0.y, v0.z);
		fprintf(stderr, "v1: %f %f %f\n", v1.x, v1.y, v1.z);
		fprintf(stderr, "ray_dir: %f %f %f\n", ray_dir.x, ray_dir.y, ray_dir.z);
		fprintf(stderr, "max_ray_len: %f\n", max_ray_len);

		float v0mul = 2.0f;
		err = clSetKernelArg(planekernels[i], 2, sizeof(cl_float4), &origin);
		CHECK_ERR("clSetKernelArg");
		err = clSetKernelArg(planekernels[i], 3, sizeof(cl_float4), &v0);
		CHECK_ERR("clSetKernelArg");
		err = clSetKernelArg(planekernels[i], 4, sizeof(cl_float4), &v1);
		CHECK_ERR("clSetKernelArg");
		err = clSetKernelArg(planekernels[i], 5, sizeof(cl_float4), &ray_dir);
		CHECK_ERR("clSetKernelArg");
		err = clSetKernelArg(planekernels[i], 6, sizeof(cl_float), &max_ray_len);
		CHECK_ERR("clSetKernelArg");
		err = clSetKernelArg(planekernels[i], 7, sizeof(cl_float), &v0mul);
		CHECK_ERR("clSetKernelArg");
	}

	err = clSetKernelArg(kernel2, 0, sizeof(cl_mem), &input_image);
	CHECK_ERR("clSetKernelArg");
	err = clSetKernelArg(kernel2, 1, sizeof(cl_mem), &output_image);
	CHECK_ERR("clSetKernelArg");

	clFinish(command_queue);
	CHECK_ERR("clFinish");

	/* now... execute it */
	measure_once("Execution and reading back", "clEnqueueNDRangeKernel", {
		cl_event plane_events[planekernels_n];
		size_t workGroupSize[2] = { sqrtf(im_in->size_x*im_in->size_x + im_in->size_y*im_in->size_y) / 2.0f, im_in->size_z };
		for (int i = 0; i < planekernels_n; i++) {
			err = clEnqueueNDRangeKernel(command_queue, planekernels[i], 2, NULL, workGroupSize, NULL, 0, NULL, &plane_events[i]);
			CHECK_ERR("clEnqueueNDRangeKernel");
		}

		size_t wgs2[3] = { (im_in->size_x + im_in->size_y) * 2, im_in->size_z, 1 };
		//err = clEnqueueNDRangeKernel(command_queue, kernel2, 3, NULL, wgs2, NULL, 0, NULL, &event);
		CHECK_ERR("clEnqueueNDRangeKernel");

		//clWaitForEvents(1, &event); CHECK_ERR("clWaitForEvents");
		clWaitForEvents(planekernels_n, plane_events); CHECK_ERR("clWaitForEvents");

		//clReleaseEvent(event);
		for (int i = 0; i < planekernels_n; i++) {
			clReleaseEvent(plane_events[i]);
			CHECK_ERR("clReleaseEvent");
		}
		clFinish(command_queue);
		CHECK_ERR("clFinish");
	});

	measure_once("Execution and reading back", "clEnqueueReadImage", {
		size_t r_origin[3] = { 0, 0, 0 };
		size_t r_region[3] = { im_in->size_x, im_in->size_y, im_in->size_z };
		err = clEnqueueReadImage(command_queue, output_image, CL_TRUE, r_origin, r_region, 0, 0, rgb_out, 0, NULL, &event);
		CHECK_ERR("clEnqueueReadImage");
		clReleaseEvent(event);
	});

	/* TODO free what we can free... like rgb_in at this point */

	/* read into output image */
	struct i3d_binary *im_out = i3d_binary_new();
	im_out->size_x = im_in->size_x;
	im_out->size_y = im_in->size_y;
	im_out->size_z = im_in->size_z;
	i3d_binary_alloc(im_out);
	for (int z = 0; z < im_in->size_z; z++) {
		for (int y = 0; y < im_in->size_y; y++) {
			for (int x = 0; x < im_in->size_x; x++) {
				int i = x + y*im_in->size_x + z*im_in->size_y*im_in->size_x;
				/* components are all the same anyway, just read R */
				i3d_binary_at(im_out, x, y, z) = rgb_out[i*4 + 0];
			}
		}
	}

	/* TODO properly free everything, including all the opencl contexts
	 * or at least the buffers */
	free(rgb_in); /* HACK */
	free(rgb_out); /* HACK HACK */

	return im_out;
}

static inline int inside_v4si(__v4si p, __v4si imdim) {
	__v4si out_of_bounds = __builtin_ia32_pcmpgtd128(p, imdim);
	__v4si nul = { 0, 0, 0, 0 };
	__v4si negative = __builtin_ia32_pcmpgtd128(nul, p);
	return
		__builtin_ia32_ptestz128((__v2di)negative, (__v2di)negative)
		&&
		__builtin_ia32_ptestz128((__v2di)out_of_bounds, (__v2di)out_of_bounds);
}

static inline uint8_t _atv4sf(struct i3d_binary *im, __v4sf p, __v4si imdim, __v4si imoff) {
	__v4si coord = __builtin_ia32_cvttps2dq(p);
	if (inside_v4si(coord, imdim)) {
		__v4si offs = imoff * coord;
		return im->voxels[offs[0] + offs[1] + offs[2] + offs[3]];
	} else {
		return 0x00;
	}
}
#define atv4sf(im,p) _atv4sf((im),(p),imdim,imoff,nul)
static inline void _tov4sf(struct i3d_binary *im, __v4sf p, uint8_t v, __v4si imdim, __v4si imoff) {
	__v4si coord = __builtin_ia32_cvttps2dq(p);
	if (inside_v4si(coord, imdim)) {
		__v4si offs = imoff * coord;
		im->voxels[offs[0] + offs[1] + offs[2] + offs[3]] = v;
	}
}
#define tov4sf(im,p,v) _tov4sf((im),(p),(v),imdim,imoff,nul)
static inline uint8_t _copyretv4si(struct i3d_binary *dst, struct i3d_binary *src, __v4si coord, __v4si imdim, __v4si imoff) {
	if (inside_v4si(coord, imdim)) {
		__v4si offs = imoff * coord;
		int off = offs[0] + offs[1] + offs[2] + offs[3];
		uint8_t v = src->voxels[off];
		dst->voxels[off] = v;
		return v;
	} else {
		return 0x00;
	}
}
#define copyretv4sf(dst,src,p) _copyretv4si((dst),(src),__builtin_ia32_cvttps2dq(p),imdim,imoff)

struct i3d_binary *cpu_raycast(struct i3d_binary *im_in) {
	struct i3d_binary *im_out = i3d_binary_new();
	im_out->size_x = im_in->size_x;
	im_out->size_y = im_in->size_y;
	im_out->size_z = im_in->size_z;
	i3d_binary_alloc(im_out);
	memset(im_out->voxels, 0xff, im_out->size_z * im_out->off_z);

	register __v4si imdim = { im_out->size_x - 1, im_out->size_y - 1, im_out->size_z - 1, 0 };
	register __v4si imoff = { 1, im_out->off_y, im_out->off_z, 0 };

	const int n_planes = 5;
	for (int i = 0; i < n_planes; i++) { measure_once("one", "iter", {
		float angle = M_PI * 2.0f * (float)i / n_planes;
		__v4sf ray_dir = { cosf(angle), sinf(angle), 0.0f, 0.0f };
		__v4sf v0 = { -ray_dir[1], ray_dir[0], 0.0f, 0.0f };
		__v4sf v1 = { 0.0f, 0.0f, 1.0f, 0.0f };
		float max_ray_len = sqrtf(im_in->size_x*im_in->size_x + im_in->size_y*im_in->size_y);
		__v4sf origin = {
			im_in->size_x*0.5f - ray_dir[0]*max_ray_len*0.5f - v0[0]*0.5f*max_ray_len,
			im_in->size_y*0.5f - ray_dir[1]*max_ray_len*0.5f - v0[1]*0.5f*max_ray_len,
			0.0f,
			0.0f
		};

		fprintf(stderr, "Origin: %f %f %f\n", origin[0], origin[1], origin[2]);
		fprintf(stderr, "v0: %f %f %f\n", v0[0], v0[1], v0[2]);
		fprintf(stderr, "v1: %f %f %f\n", v1[0], v1[1], v1[2]);
		fprintf(stderr, "ray_dir: %f %f %f\n", ray_dir[0], ray_dir[1], ray_dir[2]);
		fprintf(stderr, "max_ray_len: %f\n", max_ray_len);

		const float ray_step_size = 0.5f;
		__v4sf d_step = ray_dir * ray_step_size;
		int iterations = max_ray_len / ray_step_size;
		for (int z = 0; z < im_in->size_z; z++) {
			__v4sf dv1 = origin + v1*(float)z;
			for (int x = 0; x < max_ray_len; x+=2) {
				__v4sf startpos = v0*(float)x + dv1;
				for (int rp = 0; rp < iterations; rp++) {
					__v4sf p5;
					p5 = startpos + d_step*(float)rp;
					/* testing against lower and right edges takes much more time
					 * than just the other edge.
					 */
					__v4si lol = __builtin_ia32_cvttps2dq(p5);
					if (!inside_v4si(lol, imdim)) continue;
					uint8_t r5 = _copyretv4si(im_out,im_in,lol,imdim,imoff);

					__v4sf p2, p8;
					p2 = p5 - v1;
					p8 = p5 + v1;

					/*
					uint8_t r1 = copyretv4sf(im_out, im_in, p2 - v0);
					uint8_t r2 = copyretv4sf(im_out, im_in, p2);
					uint8_t r3 = copyretv4sf(im_out, im_in, p2 + v0);
					uint8_t r4 = copyretv4sf(im_out, im_in, p5 - v0);
					uint8_t r6 = copyretv4sf(im_out, im_in, p5 + v0);
					uint8_t r7 = copyretv4sf(im_out, im_in, p8 - v0);
					uint8_t r8 = copyretv4sf(im_out, im_in, p8);
					uint8_t r9 = copyretv4sf(im_out, im_in, p8 + v0);
					*/
					uint8_t r1 = copyretv4sf(im_out, im_in, p5 - v0*0.5f - v1 * 0.5f);
					uint8_t r2 = copyretv4sf(im_out, im_in, p5 - v0*0.5f + v1 * 0.5f);
					uint8_t r3 = copyretv4sf(im_out, im_in, p5 + v0*0.5f - v1 * 0.5f);
					uint8_t r4 = copyretv4sf(im_out, im_in, p5 + v0*0.5f + v1 * 0.5f);

					if (r1 || r2 || r3 || r4 /* || r5 || r6 || r7 || r8 || r9 */) {
						/*for (int dz = -2; dz <= +2; dz++) {
							for (int dy = -2; dy <= +2; dy++) {
								for (int dx = -2; dx <= +2; dx++) {
									__v4sf pp = p5 + v0*(float)dx + v1*(float)dy + ray_dir*(float)dz;
									tov4sf(im_out, pp, atv4sf(im_in, pp));
								}
							}
						}*/
						break;
					}
				}
			}
		}
	}); }

	return im_out;
}




