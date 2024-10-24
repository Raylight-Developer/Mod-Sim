#pragma once

#include "Shared.hpp"

#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "OpenGL.hpp"
#include "Kernel.hpp"

#include "Rasterizer.hpp"
#include "PathTracer.hpp"

enum struct Mode {
	PATHTRACING,
	RASTERIZATION
};

struct Renderer {
	GLFWwindow* window;
	Kernel kernel;

	Rasterizer rasterizer;
	PathTracer pathtracer;

	Mode render_mode;

	Transform camera_transform;

	dvec1 display_aspect_ratio;
	dvec1 render_aspect_ratio;
	uvec2 display_resolution;
	uvec2 render_resolution;

	uint64 runframe;

	uint   frame_counter;
	dvec1  frame_timer;
	dvec1  sim_timer;

	uint   frame_count;
	dvec1  frame_time;
	dvec1  sim_time;

	dvec1 camera_zoom_sensitivity;
	dvec1 camera_orbit_sensitivity;
	vector<bool> inputs;

	dvec1 sim_time_aggregate;

	dvec1 current_time;
	dvec1 window_time;
	dvec1 delta_time;
	dvec1 sim_delta;
	dvec1 last_time;

	bool  run_sim;
	bool  lock_settings;

	unordered_map<string, float> params_float;
	unordered_map<string, bool> params_bool;
	unordered_map<string, int> params_int;

	Renderer();
	~Renderer();

	void init();

	void initGlfw();
	void initImGui();
	void systemInfo();

	void f_pipeline();
	void f_recompile();
	void f_tickUpdate();

	void f_frameUpdate();
	void f_inputLoop();
	void f_timings();

	void f_changeSettings();

	void f_guiLoop();
	void f_displayLoop();

	void f_resize();

	static void framebufferSize(GLFWwindow* window, int width, int height);
	static void mouseButton(GLFWwindow* window, int button, int action, int mods);
	static void scroll(GLFWwindow* window, double xoffset, double yoffset);
	static void key(GLFWwindow* window, int key, int scancode, int action, int mods);
};