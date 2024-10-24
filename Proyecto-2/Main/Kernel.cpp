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

void Kernel::init(const unordered_map<string, float>& params_float, const unordered_map<string, bool>& params_bool,const unordered_map<string, int>& params_int) {
	this->params_float = params_float;
	this->params_bool = params_bool;
	this->params_int = params_int;

	PARTICLE_RADIUS  = params_float.at("PARTICLE_RADIUS");
	PARTICLE_COUNT   = params_int.at("PARTICLE_COUNT");
	MAX_OCTREE_DEPTH = params_int.at("MAX_OCTREE_DEPTH");
	POLE_BIAS        = params_float.at("POLE_BIAS");
	POLE_BIAS_POWER  = params_float.at("POLE_BIAS_POWER");
	POLE_GEOLOCATION = vec2(params_float.at("POLE_GEOLOCATION.x"), params_float.at("POLE_GEOLOCATION.y"));
	PARTICLE_AREA    = 4.0f * glm::pi<vec1>() * PARTICLE_RADIUS * PARTICLE_RADIUS;
	SMOOTH_RADIUS    = 1.0f * 1.5f;
	DT               = 0.016f;
	RUNFRAME         = 0;
	SAMPLES          = 5;
	SDT              = 0.016f / u_to_f(SAMPLES);

	initParticles();
	initBvh();
}

void Kernel::initParticles() {
	const vec1 radius = 6.371f;

	particles.clear();
	for (uint j = 0; j < PARTICLE_COUNT; j++) {
		CPU_Particle particle = CPU_Particle();
		const vec1 normalized_i = j / (vec1)(PARTICLE_COUNT - 1);
		const vec1 biased_i = (1.0f - POLE_BIAS) * normalized_i + POLE_BIAS * pow(normalized_i, POLE_BIAS_POWER);

		const vec1 theta = acos(1.0f - 2.0f * biased_i);
		const vec1 phi = vec1(j) * (glm::pi<vec1>() * (3.0f - sqrt(5.0f)));

		const vec1 x = radius * sin(theta) * cos(phi);
		const vec1 y = radius * cos(theta);
		const vec1 z = radius * sin(theta) * sin(phi);

		const vec3 earth_pos = vec3(params_float["EARTH_CENTER.x"], params_float["EARTH_CENTER.y"], params_float["EARTH_CENTER.z"]);

		particle.position = earth_pos + rotateGeoloc(vec3(x, y, z), POLE_GEOLOCATION);

		traceProperties(&particle);
		particles.push_back(particle);
	}
}

void Kernel::initBvh() {
	const uint bvh_depth = d_to_u(glm::log2(ul_to_d(particles.size()) / 64.0));

	const Builder bvh_build = Builder(particles, PARTICLE_RADIUS, MAX_OCTREE_DEPTH);
	particles = bvh_build.particles;
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

	const vec3 intersectionPoint = particle->position + ((-b - sqrt(delta)) / (2.0f * a)) * ray_direction;
	const vec3 normal = glm::normalize(intersectionPoint);

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

vec3 Kernel::rotateGeoloc(const vec3& point, const vec2& geoloc) {
	const vec1 phi = glm::radians(geoloc.x - 90.0f);
	const vec1 theta = glm::radians(geoloc.y + 90.0f);
	const vec1 axialTilt = -glm::radians(params_float["EARTH_TILT"]);

	const quat tiltRotation = glm::angleAxis(axialTilt, glm::vec3(0, 0, 1));
	const quat latRotation  = glm::angleAxis(phi, glm::vec3(1, 0, 0));
	const quat lonRotation  = glm::angleAxis(theta, glm::vec3(0, 1, 0));
	const quat combinedRotation = tiltRotation * lonRotation * latRotation;

	return combinedRotation * point;
}

vec3 getEarthPosition(const vec1& time, const vec1& radius) {
	const vec1 theta = glm::two_pi<float>() * (time / 365.0f);

	const vec1 x = radius * glm::cos(theta);
	const vec1 y = radius * 0.0f;
	const vec1 z = radius * glm::sin(theta);

	return vec3(x, y, z);
}