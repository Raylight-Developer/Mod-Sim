#include "Kernel.hpp"

#include "Lut.hpp"

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

Kernel::Kernel() {
	PARTICLE_RADIUS    = 0;
	PARTICLE_COUNT     = 0;
	LAYER_COUNT        = 0;
	MAX_OCTREE_DEPTH   = 0;
	POLE_BIAS          = 0;
	POLE_BIAS_POWER    = 0;
	POLE_GEOLOCATION   = vec2(0);
	PARTICLE_AREA      = 0;
	SMOOTH_RADIUS      = 0;
	DT                 = 0;
	RUNFRAME           = 0;
	SAMPLES            = 0;
	SDT                = 0;

	textures[Texture_Field::TOPOGRAPHY] = Texture::fromFile("./Resources/Nasa Earth Data/Topography.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SST] = Texture::fromFile("./Resources/Nasa Earth Data/Sea Surface Temperature CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::LST] = Texture::fromFile("./Resources/Nasa Earth Data/Land Surface Temperature CAF.png", Texture_Format::MONO_FLOAT);
}

void Kernel::init(const vec1& PARTICLE_RADIUS, const uint& PARTICLE_COUNT, const uint& LAYER_COUNT, const uint& MAX_OCTREE_DEPTH, const vec1& POLE_BIAS, const vec1& POLE_BIAS_POWER, const vec2& POLE_GEOLOCATION) {
	this->PARTICLE_RADIUS = PARTICLE_RADIUS;
	this->PARTICLE_COUNT  = PARTICLE_COUNT;
	this->LAYER_COUNT     = LAYER_COUNT;
	this->MAX_OCTREE_DEPTH    = MAX_OCTREE_DEPTH;
	this->POLE_BIAS = POLE_BIAS;
	this->POLE_BIAS_POWER = POLE_BIAS_POWER;
	this->POLE_GEOLOCATION = POLE_GEOLOCATION;
	PARTICLE_AREA         = 4.0f * glm::pi<vec1>() * PARTICLE_RADIUS * PARTICLE_RADIUS;
	SMOOTH_RADIUS         = 1.0f * 1.5f;
	DT                    = 0.016f;
	RUNFRAME              = 0;
	SAMPLES               = 5;
	SDT                   = 0.016f / u_to_f(SAMPLES);

	initParticles();
	initBvh();
}

void Kernel::initParticles() {
	const vec1 radius = 6.371f;
	const vec1 atmosphere_thickness = 0.35f;
	const vec1 layer_distance = atmosphere_thickness / u_to_f(LAYER_COUNT);

	particles.clear();
	for (uint i = 0; i < LAYER_COUNT; i++) {
		const vec1 current_layer_radius = radius + u_to_f(i) * layer_distance;
		for (uint j = 0; j < PARTICLE_COUNT; j++) {
			CPU_Particle particle = CPU_Particle();
			const vec1 normalized_i = j / (vec1)(PARTICLE_COUNT - 1);
			const vec1 biased_i = (1.0f - POLE_BIAS) * normalized_i + POLE_BIAS * pow(normalized_i, POLE_BIAS_POWER);

			const vec1 theta = acos(1.0f - 2.0f * biased_i);
			const vec1 phi = vec1(j) * (glm::pi<vec1>() * (3.0f - sqrt(5.0f)));

			const vec1 x = current_layer_radius * sin(theta) * cos(phi);
			const vec1 y = current_layer_radius * cos(theta);
			const vec1 z = current_layer_radius * sin(theta) * sin(phi);

			particle.position = rotateGeoloc(vec3(x, y, z), POLE_GEOLOCATION);
			particle.velocity = vec3(0, -1, 0);

			particle.mass = randF(0.5, 1.0);
			if (LAYER_COUNT > 1) {
				particle.temperature = f_map(0.0f, u_to_f(LAYER_COUNT - 1), AMBIENT_TEMP, ATMOSPHERE_TEMP, u_to_f(i));
			}
			else {
				particle.temperature = AMBIENT_TEMP;
			}
			traceProperties(&particle);
			particles.push_back(particle);
		}
	}
}

void Kernel::initBvh() {
	const uint bvh_depth = d_to_u(glm::log2(ul_to_d(particles.size()) / 64.0));

	Builder bvh_build = Builder(particles, PARTICLE_RADIUS, MAX_OCTREE_DEPTH);
	particles = bvh_build.particles;
	root_node = bvh_build.gpu_root_node;
	bvh_nodes = bvh_build.nodes;

	gpu_particles.clear();
	for (const CPU_Particle& particle : particles) {
		gpu_particles.push_back(GPU_Particle(particle));
	}
}

void Kernel::simulate(const dvec1& delta_time) {
	DT = d_to_f(delta_time);
	SDT = DT / u_to_f(SAMPLES);
}

vec1 Kernel::smoothWeight(const vec1& distance) const {
	//vec1 value = glm::max(0.0f, radius * radius - distance * distance);
	//return value * value * value
	const vec1 value = glm::max(0.0f, SMOOTH_RADIUS - distance);
	const vec1 volume = glm::pi<vec1>() * pow(SMOOTH_RADIUS, 3.0f) /3.0f;
	return value;
}

void Kernel::traceProperties(CPU_Particle* particle) {
	const vec3 ray_direction = glm::normalize(vec3(0) - particle->position);

	const vec1 a = glm::dot(ray_direction, ray_direction);
	const vec1 b = 2.0f * dot(ray_direction, particle->position);
	const vec1 c = dot(particle->position, particle->position) - 40.589641f; // Earth Radius ^2
	const vec1 delta = b * b - 4.0f * a * c;
	if (delta < 0.0f) {
		return;
	}

	const vec1 axialTilt = glm::radians(23.5f);
	const mat3 tiltRotation = mat3(1.0, 0.0, 0.0, 0.0, cos(axialTilt), -sin(axialTilt), 0.0, sin(axialTilt), cos(axialTilt));

	const vec3 intersectionPoint = particle->position + ((-b - sqrt(delta)) / (2.0f * a)) * ray_direction;
	const vec3 normal = tiltRotation *glm::normalize(intersectionPoint);

	const vec1 theta = acos(normal.y);
	const vec1 phi = glm::atan(normal.z, normal.x);

	const vec2 uv = vec2(1.0 - ((phi + PI) / TWO_PI), (theta) / PI);

	const vec1 topography_sample = textures[Texture_Field::TOPOGRAPHY].sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 sst_sample = textures[Texture_Field::SST].sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 lst_sample = textures[Texture_Field::LST].sampleTextureMono(uv, Texture_Format::MONO_FLOAT);

	const vec1 topography =  lut(Texture_Field::TOPOGRAPHY, topography_sample);
	const vec1 sst =         lut(Texture_Field::SST, sst_sample);
	const vec1 lst =         lut(Texture_Field::LST, lst_sample);

	if (topography == -1.0f) { // Is at sea
		particle->temperature = sst;
	}
	else { // Is on Land
		particle->temperature = lst;
	}
}

vec3 rotateGeoloc(const vec3& point, const vec2& geoloc) {
	const vec1 phi = glm::radians(geoloc.x - 90.0f) - glm::radians(23.5f);
	const vec1 theta = glm::radians(geoloc.y);

	const mat3 rotationY = mat3(
		vec3(cos(theta), 0, sin(theta)),
		vec3(0, 1, 0),
		vec3(-sin(theta), 0, cos(theta))
	);

	const mat3 rotationX = mat3(
		vec3(1, 0, 0),
		vec3(0, cos(phi), -sin(phi)),
		vec3(0, sin(phi), cos(phi))
	);

	vec3 res = point;
	res = rotationY * res;
	res = rotationX * res;

	return res;
}