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

	bool use_octree;
	bool render_planet;
	bool render_lighting;
	bool render_particle_lighting;
	bool render_atmosphere;
	bool render_octree;
	bool render_octree_hue;
	bool render_octree_debug;
	bool render_particles;
	int render_octree_debug_index;
	int render_particle_color_mode;
	int render_planet_texture;
	float render_particle_radius;

	PathTracer(Renderer* renderer = nullptr);

	void f_initialize();

	void f_updateBvh();
	void f_updateParticles();
	void f_updateTextures(const bool& high_res);

	void f_guiUpdate();
	void f_recompile();
	void f_cleanup();
	void f_resize();

	void f_render();
};