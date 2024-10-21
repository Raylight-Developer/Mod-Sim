#pragma once

#include "Shared.hpp"

#include "OpenGl.hpp"

enum struct Texture_Field;

struct CPU_Particle {
	vec1 mass;
	vec1 density;
	vec1 temperature;

	vec1 sea_surface_temperature;

	vec3 position;
	vec3 velocity;
	vec3 acceleration;

	CPU_Particle();
};

struct alignas(16) GPU_Particle {
	vec3 position;
	vec1 temperature;
	vec3 velocity;
	vec1 sea_surface_temperature;

	GPU_Particle();
	GPU_Particle(const CPU_Particle& particle);
};

struct alignas(16) GPU_Texture {
	uint start;
	uint width;
	uint height;
	uint format;

	GPU_Texture(
		const uint& start = 0U,
		const uint& width = 0U,
		const uint& height = 0U,
		const uint& format = 0U
	);
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
	unordered_map<Texture_Field, Texture> textures;

	Flip();

	void init(const vec1& PARTICLE_RADIUS, const uint& PARTICLE_COUNT, const uint& LAYER_COUNT);
	void initParticles();

	void simulate(const dvec1& delta_time);

	vector<GPU_Particle> gpuParticles() const;
	vec1 smoothWeight(const vec1& distance) const;

	void traceProperties(CPU_Particle* particle);
};