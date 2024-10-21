#pragma once

#include "Shared.hpp"

struct CPU_Cell;

struct CPU_Particle {
	vec1 mass;
	vec1 density;
	vec1 temperature;

	vec3 position;
	vec3 velocity;
	vec3 acceleration;

	CPU_Particle();
};

struct alignas(16) GPU_Particle {
	vec3 position;
	vec1 temperature;
	vec4 velocity;

	GPU_Particle();
	GPU_Particle(const CPU_Particle& particle);
};

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

	Flip();

	void init(const vec1& PARTICLE_RADIUS, const uint& PARTICLE_COUNT, const uint& LAYER_COUNT);
	void initParticles();

	void simulate(const dvec1& delta_time);

	vector<GPU_Particle> gpuParticles() const;
	vec1 smoothWeight(const vec1& distance) const;
};