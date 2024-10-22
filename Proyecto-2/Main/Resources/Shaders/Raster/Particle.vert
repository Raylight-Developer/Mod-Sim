#version 460

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

out vec2 fragUV;
out vec3 fragPos;
out vec3 fragNormal;

uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat4 projection_matrix;

void main() {
	fragUV = aUV;
	fragPos = mat3(model_matrix) * aPos;
	fragNormal = mat3(model_matrix) * aNormal;

	gl_Position = projection_matrix * view_matrix * model_matrix * vec4(aPos, 1.0);
}