#pragma once

#include "Shared.hpp"

#include "OpenGL.hpp"

#define SPH 20

struct Renderer;

struct PathTracer {
	Renderer* renderer;

	unordered_map<string, GLuint> gl_data;
	uvec2 compute_layout;

	vector<GPU_Texture> textures;
	uint texture_size;

	bool render_planet;
	bool render_lighting;

	bool use_probe_octree;
	bool use_particle_octree;
	bool render_probes;
	bool render_particles;
	bool render_probe_lighting;
	bool render_particle_lighting;
	bool render_probe_octree;
	bool render_particle_octree;

	bool render_atmosphere;
	bool render_octree_hue;
	bool render_octree_debug;
	int  render_octree_debug_index;
	int  render_probe_color_mode;
	int  render_planet_texture;

	PathTracer(Renderer* renderer = nullptr);

	void f_initialize();

	void f_updateBvhProbes();
	void f_updateBvhParticles();
	void f_updateProbes();
	void f_updateParticles();
	void f_updateTextures(const bool& high_res);

	void f_guiUpdate(const vec1& availableWidth, const vec1& spacing, const vec1& itemWidth, const vec1& halfWidth, const vec1& thirdWidth, const vec1& halfPos);
	void f_recompile();
	void f_cleanup();
	void f_resize();

	void f_render();
};