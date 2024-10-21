#pragma once

#include "Shared.hpp"

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