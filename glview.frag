#version 130

in vec3 VerPos;
in vec3 VerNor;

varying vec3 normal, normalCamspace;

uniform mat4 mat_proj, mat_view;

void main() {
	gl_FragColor = vec4(vec3(clamp(
		/* 50% generic, non-directional ambient light */
		0.5
		+
		/* 50% light directly from camera */
		0.5 * clamp(
			dot(normalCamspace, vec3(0.0, 0.0, 1.0))
		, 0.0, 1.0)
	, 0.0, 1.0)), 1.0);
}

