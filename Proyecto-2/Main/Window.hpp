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

	vector<GPU_Texture> textures;

	dvec1 sim_time_aggregate;

	dvec1 current_time;
	dvec1 window_time;
	dvec1 delta_time;
	dvec1 sim_delta;
	dvec1 last_time;

	unordered_map<string, GLuint> buffers;

	bool  run_sim;

	vec1  TIME_SCALE;
	vec1  RENDER_SCALE;
	vec1  PARTICLE_RADIUS;
	vec1  PARTICLE_DISPLAY;
	int   PARTICLE_COUNT;
	int   LAYER_COUNT;
	int   OCTREE_DEPTH;

	uvec3 compute_layout;

	bool render_planet;
	bool render_octree;
	bool render_particles;
	int  render_particle_color_mode;

	int  render_planet_texture;

	Renderer();
	~Renderer();

	void init();

	void initGlfw();
	void initImGui();
	void systemInfo();

	void f_pipeline();
	void f_tickUpdate();

	void initKernel();

	void guiLoop();
	void gameLoop();
	void displayLoop();

	void resize();

	static void framebufferSize(GLFWwindow* window, int width, int height);
	static void mouseButton(GLFWwindow* window, int button, int action, int mods);
	static void scroll(GLFWwindow* window, double xoffset, double yoffset);
	static void key(GLFWwindow* window, int key, int scancode, int action, int mods);
};