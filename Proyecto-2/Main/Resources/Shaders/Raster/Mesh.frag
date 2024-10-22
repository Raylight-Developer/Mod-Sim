#version 460

in vec2 fragUV;
in vec3 fragPos;
in vec3 fragNormal;

out vec4 fragColor;

uniform vec3 camera_pos;
uniform uint wireframe;
uniform uint stencil;

vec3 f_lambert(vec3 lightDir, vec3 lightColor, vec3 objectColor) {
	float diff = max(dot(fragNormal, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;
	return diffuse * objectColor;
}

float f_fresnel(float cosi, float ior) {
	float c = abs(cosi);
	float g = ior * ior - 1 + c * c;
	float result;

	if (g > 0) {
		g = sqrt(g);
		float A = (g - c) / (g + c);
		float B = (c * (g + c) - 1) / (c * (g - c) + 1);
		result = 0.5 * A * A * (1 + B * B);
	}
	else {
		result = 1.0;
	}
	return result;
}

void main() {
	if (wireframe == 1) {
		fragColor = vec4(1.0, 0.625, 0.175, 0.5);
	}
	else if (stencil == 1) {
		fragColor = vec4(0.625, 1.0, 0.175, 1.0);
	}
	else {
		vec3 normal = normalize(fragNormal);
		vec3 view_dir = normalize(camera_pos - fragPos);
		vec3 light_dir = normalize(vec3(1.0, -1.0, 1.0));

		float cosi = dot(view_dir, normal);
		float fresnel = f_fresnel(cosi, 1.5);
		vec3  fresnel_color = vec3(1.0 - pow(fresnel, 0.6));

		vec3 lambert = (f_lambert(vec3(0.577350269), vec3(1), vec3(1)) * 0.8 + 0.2);

		vec3 color = fresnel_color * lambert;
		fragColor = vec4(color, 1.0);
	}
}