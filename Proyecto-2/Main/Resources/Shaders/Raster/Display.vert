#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

uniform float display_aspect_ratio;
uniform float render_aspect_ratio;

out vec2 fragUV;

void main() {
	if (display_aspect_ratio < render_aspect_ratio)
		gl_Position = vec4(aPos * vec2(1.0, display_aspect_ratio/render_aspect_ratio), 0.0, 1.0);
	else
		gl_Position = vec4(aPos * vec2(render_aspect_ratio/display_aspect_ratio, 1.0), 0.0, 1.0);
	fragUV = aUV;
}