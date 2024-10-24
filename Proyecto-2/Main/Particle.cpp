#include "Particle.hpp"

CPU_Particle::CPU_Particle() {
	position = vec3(0);
	temperature = 20.0;
}

GPU_Particle::GPU_Particle() {
	position = vec3(0);
	temperature = 20.0;
}

GPU_Particle::GPU_Particle(const CPU_Particle& particle) {
	position = particle.position;
	temperature = particle.temperature;
}