#version 460 core

uniform float display_aspect_ratio;
uniform float render_aspect_ratio;

uniform sampler2D raw_render_layer;

in vec2 uv_coords;
out vec4 color_out;

void main() {
	color_out = texture(raw_render_layer, uv_coords);
}