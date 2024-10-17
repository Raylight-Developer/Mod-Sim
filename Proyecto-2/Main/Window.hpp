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

	vector<CPU_Particle> cpu_point_cloud;
	Grid cpu_grid;

	Transform camera_transform;

	dvec1 display_aspect_ratio;
	dvec1 render_aspect_ratio;
	uvec2 display_resolution;
	uvec2 render_resolution;

	uint   frame_counter;
	uint   frame_count;
	uint64 runframe;

	bool recompile;

	dvec1 camera_zoom_sensitivity;
	dvec1 camera_orbit_sensitivity;
	vector<bool> keys;

	dvec2 current_mouse;
	dvec2 last_mouse;

	dvec1 sim_deltas;

	dvec1 sim_delta;
	dvec1 current_time;
	dvec1 window_time;
	dvec1 delta_time;
	dvec1 last_time;

	unordered_map<string, GLuint> buffers;

	Renderer();

	void init();
	void quit();

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
	static void cursorPos(GLFWwindow* window, double xpos, double ypos);
	static void mouseButton(GLFWwindow* window, int button, int action, int mods);
	static void scroll(GLFWwindow* window, double xoffset, double yoffset);
	static void key(GLFWwindow* window, int key, int scancode, int action, int mods);
};