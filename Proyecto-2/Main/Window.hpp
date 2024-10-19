#pragma once

#include "Shared.hpp"

#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "OpenGL.hpp"
#include "Kernel.hpp"

struct Renderer {
	GLFWwindow* window;
	Flip flip;

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

	bool recompile;

	dvec1 camera_zoom_sensitivity;
	dvec1 camera_orbit_sensitivity;
	vector<bool> inputs;

	dvec1 sim_time_aggregate;

	dvec1 current_time;
	dvec1 window_time;
	dvec1 delta_time;
	dvec1 sim_delta;
	dvec1 last_time;

	unordered_map<string, GLuint> buffers;

	bool start_sim;
	bool* render_grid;
	bool* render_particles;

	bool* render_grid_surface;
	bool* render_grid_density;

	vec1* render_grid_opacity;
	vec1* render_grid_density_mul;

	Renderer();
	~Renderer();

	void init();

	void initGlfw();
	void initImGui();
	void systemInfo();

	void f_pipeline();
	void f_tickUpdate();

	void guiLoop();
	void gameLoop();
	void displayLoop();

	void resize();

	static void framebufferSize(GLFWwindow* window, int width, int height);
	static void mouseButton(GLFWwindow* window, int button, int action, int mods);
	static void scroll(GLFWwindow* window, double xoffset, double yoffset);
	static void key(GLFWwindow* window, int key, int scancode, int action, int mods);
};