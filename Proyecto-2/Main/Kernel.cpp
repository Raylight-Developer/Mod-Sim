#include "Kernel.hpp"

#define WARM_REGION        0.8f    // Bottom 1-N of half simulation_height
#define COLD_REGION        0.8f    // Bottom N of half simulation_height

#define AIR_GAS_CONSTANT   287.05f // J/(kg·K)
#define AIR_DENSITY        1.225f  // kg/m^3 (at sea level)

#define PARTICLE_RESTITUTION       0.95f
#define RESTITUTION                0.95f
#define GRAVITY                    vec3(0.0)

#define ATMOSPHERE_TEMP    5.5f        // C
#define SEA_SURFACE_TEMP   28.5f       // C
#define AMBIENT_TEMP       15.5f       // C

#define AIR_SPECIFIC_HEAT_CAPACITY 1005.0f // J/kg°C
#define AIR_DENSITY                1.225f // kg/m³

#define CORIOLIS           vec3(15.0f, 0, 0)

#define CELL_HEAT_GAIN             1.0f
#define CELL_AMBIENT_HEAT_TRANSFER 0.05f
#define HEAT_TRANSFER_COEFFICIENT  0.05f

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
	velocity = vec4(0);
}

GPU_Particle::GPU_Particle(const CPU_Particle& particle) {
	position = particle.position;
	temperature = particle.temperature;
	velocity = vec4(particle.velocity, 1);
}

Flip::Flip() {
}

void Flip::init(const vec1& PARTICLE_RADIUS, const uint& PARTICLE_COUNT, const uint& LAYER_COUNT) {
	this->PARTICLE_RADIUS = PARTICLE_RADIUS;
	this->PARTICLE_COUNT  = PARTICLE_COUNT;
	this->LAYER_COUNT     = LAYER_COUNT;
	PARTICLE_AREA         = 4.0f * glm::pi<vec1>() * PARTICLE_RADIUS * PARTICLE_RADIUS;
	SMOOTH_RADIUS         = 1.0f * 1.5f;
	DT                    = 0.016f;
	RUNFRAME              = 0;
	SAMPLES               = 5;
	SDT                   = 0.016f / u_to_f(SAMPLES);

	initParticles();
}

void Flip::initParticles() {
	const vec1 radius = 6.371f;
	const vec1 atmosphere_thickness = 0.35f;
	const vec1 layer_distance = atmosphere_thickness / u_to_f(LAYER_COUNT);

	particles.clear();
	for (uint i = 0; i < LAYER_COUNT; i++) {
		const vec1 current_layer_radius = radius + u_to_f(i) * layer_distance;
		for (uint j = 0; j < PARTICLE_COUNT; j++) {
			CPU_Particle particle = CPU_Particle();
			const vec1 normalized_i = j / (vec1)(PARTICLE_COUNT - 1);

			const vec1 theta = acos(1.0f - 2.0f * normalized_i);
			const vec1 phi = vec1(j) * (glm::pi<vec1>() * (3.0f - sqrt(5.0f)));

			const vec1 x = current_layer_radius * sin(theta) * cos(phi);
			const vec1 y = current_layer_radius * cos(theta);
			const vec1 z = current_layer_radius * sin(theta) * sin(phi);

			particle.position = vec3(x, z, y);
			particle.velocity = vec3(0, -1, 0);

			particle.mass = randF(0.5, 1.0);
			if (LAYER_COUNT > 1) {
				particle.temperature = f_map(0.0f, u_to_f(LAYER_COUNT - 1), AMBIENT_TEMP, ATMOSPHERE_TEMP, u_to_f(i));
			}
			else {
				particle.temperature = AMBIENT_TEMP;
			}
			particles.push_back(particle);
		}
	}
}

void Flip::simulate(const dvec1& delta_time) {
	DT = d_to_f(delta_time);
	SDT = DT / u_to_f(SAMPLES);
}

vector<GPU_Particle> Flip::gpuParticles() const {
	vector<GPU_Particle> gpu;
	for (const CPU_Particle& particle : particles) {
		gpu.push_back(GPU_Particle(particle));
	}
	return gpu;
}

vec1 Flip::smoothWeight(const vec1& distance) const {
	//vec1 value = glm::max(0.0f, radius * radius - distance * distance);
	//return value * value * value
	const vec1 value = glm::max(0.0f, SMOOTH_RADIUS - distance);
	const vec1 volume = glm::pi<vec1>() * pow(SMOOTH_RADIUS, 3.0f) /3.0f;
	return value;
}