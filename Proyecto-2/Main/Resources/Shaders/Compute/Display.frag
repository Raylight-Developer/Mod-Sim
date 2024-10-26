#version 460 core

uniform float display_aspect_ratio;
uniform float render_aspect_ratio;

uniform uvec2 display_resolution;
uniform uvec2 render_resolution;

uniform sampler2D raw_render_layer;
uniform sampler2D pass_1;
uniform sampler2D pass_2;
uniform sampler2D pass_3;
uniform sampler2D pass_4;
uniform sampler2D pass_W;

in  vec2 uv_coords;
out vec4 color_out;

void main() {
	color_out = texture(raw_render_layer, uv_coords);
}