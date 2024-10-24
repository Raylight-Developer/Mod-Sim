#include "Particle.hpp"

CPU_Particle::CPU_Particle() {
	position = vec3(0);
	temperature = 20.0;

	smoothing_radius = 0.0f;
}

GPU_Particle::GPU_Particle() {
	position = vec3(0);
	temperature = 20.0;
}

GPU_Particle::GPU_Particle(const CPU_Particle& particle) {
	position = particle.position;
	temperature = particle.temperature;
}

CPU_Neighbor::CPU_Neighbor(const vec1& distance, CPU_Particle* neighbor) :
	distance(distance),
	neighbor(neighbor)
{}