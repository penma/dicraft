__constant sampler_t sampler =
	CLK_NORMALIZED_COORDS_FALSE |
	CLK_ADDRESS_CLAMP_TO_EDGE |
	CLK_FILTER_NEAREST;

#define ray_step_size 0.5f

__kernel void plane(
	__read_only image3d_t im_in,
	__write_only image3d_t im_out,
	float4 origin, /* was: (imcenter.xy 0 0) - ray_dir*max_ray_len/2 - v0/2 */
	float4 v0, /* was: -ray_dir.y ray_dir.x 0 0 */
	float4 v1, /* was: z */
	float4 ray_dir, /* was: cos/sin(angle) 0 0 */
	float max_ray_len, /* was: length(im_dim.xy) */
	float v0mul /* v0 multiplier for startpos */
) {
	float4 startpos = origin + v0*v0mul*get_global_id(0) + v1*get_global_id(1);
	float4 d_step = ray_dir * ray_step_size;
	int iterations = max_ray_len / ray_step_size;
	float4 p[9];
	for (int rp = 0; rp < iterations; rp++) {
		p[4] = startpos + d_step*rp;
		/* testing against lower and right edges takes much more time
		 * than just the other edge.
		 */
		if (p[4].x < 0 || p[4].y < 0) continue;

		p[3] = p[4] - v0;
		p[5] = p[4] + v0;

		p[0] = p[3] - v1;
		p[1] = p[4] - v1;
		p[2] = p[5] - v1;

		p[6] = p[3] + v1;
		p[7] = p[4] + v1;
		p[8] = p[5] + v1;

		bool hit = 0;
		for (int i = 0; i < 9; i++) {
			uint4 r = read_imageui(im_in, sampler, p[i]);
			if (r.x != 0) hit = 1;
			write_imageui(im_out, p[i], r);
		}

		if (hit) {
			float4 pp0 = p[4] + ray_dir;
			float4 pp1 = pp0 + v0;
			float4 pp2 = pp0 - v0;
			write_imageui(im_out, pp0, read_imageui(im_in, sampler, pp0));
			write_imageui(im_out, pp1, read_imageui(im_in, sampler, pp1));
			write_imageui(im_out, pp2, read_imageui(im_in, sampler, pp2));
			break;
		}
	}
}

__kernel void part2(__read_only image3d_t im_in, __write_only image3d_t im_out) {
	int4 dim = get_image_dim(im_in);
	float4 dimf = (float4)(dim.x, dim.y, dim.z, 0.0f);
	float diagp = length(dimf.xy);

	int angle_id = get_global_id(0);
	float angle = radians(angle_id * 360.0f / get_global_size(0));
	int zpos = get_global_id(1);

	int max_ray_len = diagp * 0.5f;

	float4 ray_dir = (float4)(cos(angle), sin(angle), 0.0f, 0.0f);
	float4 d_shift = (float4)(-ray_dir.y, ray_dir.x, 0.0f, 0.0f);
	float4 startpos = (float4)(dimf.x * 0.5f, dimf.y * 0.5f, zpos, 0.0f);
	float4 d_step = ray_dir * ray_step_size;
	int iter = max_ray_len / ray_step_size;
	for (int rp = 0; rp < iter; rp++) {
		float4 p0 = startpos + d_step*rp;

		float4 p1 = p0 + d_shift * 1.0f;
		float4 p2 = p0 + d_shift * -1.0f;
		uint4 r0 = read_imageui(im_in, sampler, p0);
		uint4 r1 = read_imageui(im_in, sampler, p1);
		uint4 r2 = read_imageui(im_in, sampler, p2);

		write_imageui(im_out, p0, r0);
		write_imageui(im_out, p1, r1);
		write_imageui(im_out, p2, r2);

		if (r0.x != 0 || r1.x != 0 || r2.x != 0) {
			float4 pp0 = p0 + ray_dir * 1.0f;
			float4 pp1 = pp0 + d_shift * 1.0f;
			float4 pp2 = pp0 + d_shift * -1.0f;
			write_imageui(im_out, pp0, read_imageui(im_in, sampler, pp0));
			write_imageui(im_out, pp1, read_imageui(im_in, sampler, pp1));
			write_imageui(im_out, pp2, read_imageui(im_in, sampler, pp2));
			break;
		}
	}
}
