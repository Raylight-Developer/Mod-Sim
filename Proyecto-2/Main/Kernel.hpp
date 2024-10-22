#pragma once

#include "Shared.hpp"

#include "OpenGl.hpp"

#include "Particle.hpp"
#include "Bvh.hpp"
#include "GPU_Debug.hpp"

enum struct Texture_Field;

struct Kernel {
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
	vector<GPU_Bvh> bvh_nodes;
	GPU_Bvh root_node;
	unordered_map<Texture_Field, Texture> textures;

	Kernel();

	void init(const vec1& PARTICLE_RADIUS, const uint& PARTICLE_COUNT, const uint& LAYER_COUNT, const uint& MAX_OCTREE_DEPTH, const vec1& POLE_BIAS, const vec1& POLE_BIAS_POWER, const vec2& POLE_GEOLOCATION);
	void initParticles();
	void initBvh();

	void simulate(const dvec1& delta_time);

	vector<GPU_Particle> gpuParticles() const;
	vec1 smoothWeight(const vec1& distance) const;

	void traceProperties(CPU_Particle* particle);
	void debug();
};

vec3 rotateGeoloc(const vec3& point, const vec2& geoloc);