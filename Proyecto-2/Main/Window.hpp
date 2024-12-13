#pragma once

#include "Shared.hpp"

#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "OpenGL.hpp"
#include "Kernel.hpp"

#include "PathTracer.hpp"

struct Renderer {
	GLFWwindow* window;
	Kernel kernel;

	PathTracer pathtracer;

	Transform camera_transform;
	dquat world_rot;
	dquat camera;
	dvec1 zoom;

	dvec1 display_aspect_ratio;
	dvec1 render_aspect_ratio;
	uvec2 display_resolution;
	uvec2 render_resolution;
	vec1  render_scale;

	uint64 runframe;

	uint   frame_counter;
	dvec1  frame_timer;
	dvec1  gpu_timer;

	uint   frame_count;
	dvec1  frame_time;
	dvec1  gpu_time;

	dvec1 camera_zoom_sensitivity;
	dvec1 camera_orbit_sensitivity;

	dvec1 input_lerp;
	vector<bool> inputs;
	vector<dvec3> input_lerps; // current , progress, target

	dvec1 gpu_time_aggregate;

	chrono::high_resolution_clock::time_point start_time;
	chrono::high_resolution_clock::time_point current_time;
	dvec1 window_time;
	dvec1 delta_time;
	dvec1 gpu_delta;
	chrono::high_resolution_clock::time_point last_time;

	bool  run_sim;
	bool  next_frame;
	bool  lock_settings;
	bool  lock_view;

	Renderer();
	~Renderer();

	void init();
	void initGlfw();
	void initImGui();
	void systemInfo();

	void f_pipeline();
	void f_tickUpdate();

	void f_frameUpdate();
	void f_inputLoop();
	void f_timings();

	void f_updateProbes();
	void f_updateParticles();

	void f_guiLoop();
	void f_displayLoop();

	void f_resize();

	static void framebufferSize(GLFWwindow* window, int width, int height);
	static void mouseButton(GLFWwindow* window, int button, int action, int mods);
	static void scroll(GLFWwindow* window, double xoffset, double yoffset);
	static void key(GLFWwindow* window, int key, int scancode, int action, int mods);
};