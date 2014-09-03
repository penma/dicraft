#version 130

in vec3 VerPos;
in vec3 VerNor;

varying vec3 normal, normalCamspace;

uniform mat4 mat_proj, mat_view;

void main() {
	normal = VerNor;
	normalCamspace = (mat_view * vec4(VerNor, 0.0)).xyz;
	gl_Position = mat_proj * mat_view * vec4(VerPos, 1.0);
}

