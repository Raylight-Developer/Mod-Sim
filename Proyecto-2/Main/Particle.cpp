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
	temperature = 0;
	velocity = vec3(0);
}

GPU_Particle::GPU_Particle(const CPU_Particle& particle) {
	position = particle.position;
	temperature = particle.temperature;
	velocity = particle.velocity;
	sea_surface_temperature = particle.sea_surface_temperature;
}