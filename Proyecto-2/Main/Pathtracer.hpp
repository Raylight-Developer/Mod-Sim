#pragma once

#include "Shared.hpp"

#include "OpenGL.hpp"

struct Renderer;

struct PathTracer {
	Renderer* renderer;

	unordered_map<string, GLuint> gl_data;
	uvec2 compute_layout;

	vector<GPU_Texture> textures;
	uint texture_size;

	unordered_map<string, float> params_float;
	unordered_map<string, bool> params_bool;
	unordered_map<string, int> params_int;

	PathTracer(Renderer* renderer = nullptr);

	void f_initialize();
	void f_tickUpdate();
	void f_changeSettings();

	void f_guiUpdate();
	void f_recompile();
	void f_cleanup();
	void f_resize();

	void f_render();
};