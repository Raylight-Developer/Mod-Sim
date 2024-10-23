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
	vec1  PARTICLE_RADIUS;
	uint  PARTICLE_COUNT;
	uint  LAYER_COUNT;
	uint  MAX_OCTREE_DEPTH;
	vec1  POLE_BIAS;
	vec1  POLE_BIAS_POWER;
	vec2  POLE_GEOLOCATION;

	vec1  PARTICLE_AREA;
	vec1  SMOOTH_RADIUS;
	vec1  DT;
	uint  RUNFRAME;
	uint  SAMPLES;
	vec1  SDT;

	vector<CPU_Particle> particles;
	unordered_map<Texture_Field, Texture> textures;

	vector<GPU_Particle> gpu_particles;
	vector<GPU_Bvh> bvh_nodes;

	Kernel();

	void init(const unordered_map<string, float>& params_float, const unordered_map<string, bool>& params_bool,const unordered_map<string, int>& params_int);
	void initParticles();
	void initBvh();

	void simulate(const dvec1& delta_time);

	vec1 smoothWeight(const vec1& distance) const;

	void traceProperties(CPU_Particle* particle);
	vec3 rotateGeoloc(const vec3& point, const vec2& geoloc);
};