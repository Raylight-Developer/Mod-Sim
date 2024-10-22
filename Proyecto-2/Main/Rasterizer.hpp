#pragma once

#include "Shared.hpp"

#include "OpenGL.hpp"

struct Renderer;

struct Rasterizer {
	Renderer* renderer;

	unordered_map<string, GLuint> data;
	unordered_map<uint64, vector<vec1>> gl_triangle_cache;
	unordered_map<string, GLuint> gl_data;

	Rasterizer(Renderer* renderer = nullptr);

	void f_initialize();
	void f_tickUpdate();
	void f_changeSettings();

	void f_guiUpdate();
	void f_recompile();
	void f_cleanup();
	void f_resize();

	void f_render();
};