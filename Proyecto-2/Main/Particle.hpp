#pragma once

#include "Shared.hpp"

struct CPU_Particle;
struct CPU_Neighbor;

struct CPU_Particle {
	vec3 position;
	vec1 temperature;

	vec1 smoothing_radius;
	vector<CPU_Neighbor> neighbors;

	CPU_Particle();
};

struct CPU_Neighbor {
	vec1 distance;
	CPU_Particle* neighbor;

	CPU_Neighbor(const vec1& distance, CPU_Particle* neighbor);
};

struct alignas(16) GPU_Particle {
	vec3 position;
	vec1 temperature;

	GPU_Particle();
	GPU_Particle(const CPU_Particle& particle);
};