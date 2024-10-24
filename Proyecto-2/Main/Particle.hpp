#pragma once

#include "Shared.hpp"

struct CPU_Particle {
	vec3 position;
	vec1 temperature;

	CPU_Particle();
};

struct alignas(16) GPU_Particle {
	vec3 position;
	vec1 temperature;

	GPU_Particle();
	GPU_Particle(const CPU_Particle& particle);
};