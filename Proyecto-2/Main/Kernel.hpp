#pragma once

#include "Shared.hpp"

#include "OpenGl.hpp"

#include "Particle.hpp"
#include "Bvh.hpp"

enum struct Texture_Field;

struct Kernel {
	unordered_map<string, float> params_float;
	unordered_map<string, bool> params_bool;
	unordered_map<string, int> params_int;
	vec1 PARTICLE_RADIUS;
	uint PARTICLE_COUNT;
	uint MAX_OCTREE_DEPTH;
	vec1 POLE_BIAS;
	vec1 POLE_BIAS_POWER;
	vec2 POLE_GEOLOCATION;

	vec1 DT;
	uint RUNFRAME;
	uint SAMPLES;
	vec1 SDT;

	vector<CPU_Particle> particles;
	unordered_map<Texture_Field, Texture> textures;

	vector<GPU_Particle> gpu_particles;
	vector<GPU_Bvh> bvh_nodes;

	vec1  time;
	uint  frame_count;
	uvec2 compute_layout;
	uvec2 compute_resolution;
	unordered_map<string, GLuint> gl_data;

	Kernel();

	void preInit(const unordered_map<string, float>& params_float, const unordered_map<string, bool>& params_bool,const unordered_map<string, int>& params_int);
	void preInitParticles();

	void init();
	void initParticles();
	void initBvh();

	void simulate(const dvec1& delta_time);
	void f_recompile();

	void traceProperties(CPU_Particle* particle) const;
	vec3 rotateGeoloc(const vec3& point, const vec2& geoloc) const;
	vec3 sunDir() const;
};

vec1 dateToFloat(const int& month, const int& day);