#pragma once

#include "Shared.hpp"

#include "OpenGl.hpp"

#include "Particle.hpp"
#include "Bvh.hpp"

enum struct Texture_Field;

struct Flip {
	vec1  PARTICLE_RADIUS;
	vec1  PARTICLE_AREA;
	uint  PARTICLE_COUNT;
	uint  LAYER_COUNT;
	vec1  SMOOTH_RADIUS;
	vec1  DT;
	uint  RUNFRAME;
	uint  SAMPLES;
	vec1  SDT;

	vector<CPU_Particle> particles;
	vector<GPU_Bvh> bvh_nodes;
	GPU_Bvh root_node;
	unordered_map<Texture_Field, Texture> textures;

	Flip();

	void init(const vec1& PARTICLE_RADIUS, const uint& PARTICLE_COUNT, const uint& LAYER_COUNT);
	void initParticles();
	void initBvh();

	void simulate(const dvec1& delta_time);

	vector<GPU_Particle> gpuParticles() const;
	vec1 smoothWeight(const vec1& distance) const;

	void traceProperties(CPU_Particle* particle);
};