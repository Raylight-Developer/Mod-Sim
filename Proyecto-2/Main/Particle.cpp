#include "Particle.hpp"

CPU_Particle::CPU_Particle() {
	mass = 0;
	density = 0;
	temperature = 0;

	position = vec3(0);
	velocity = vec3(0);
	acceleration = vec3(0);
}

GPU_Particle::GPU_Particle() {
	position = vec3(0);
}

GPU_Particle::GPU_Particle(const CPU_Particle& particle) {
	position = particle.position;
}