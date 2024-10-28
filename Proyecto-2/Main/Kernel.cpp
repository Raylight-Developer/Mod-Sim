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
	DT                 = 0;
	RUNFRAME           = 0;
	SAMPLES            = 0;
	SDT                = 0;
	time               = 0;
	frame_count        = 0;

	textures[Texture_Field::TOPOGRAPHY]                     = Texture::fromFile("./Resources/Nasa Earth Data/Topography.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::BATHYMETRY]                     = Texture::fromFile("./Resources/Nasa Earth Data/Bathymetry.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SURFACE_PRESSURE]               = Texture::fromFile("./Resources/Nasa Earth Data/MODIS/Surface Pressure CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SEA_SURFACE_TEMPERATURE_DAY]    = Texture::fromFile("./Resources/Nasa Earth Data/Sea Surface Temperature CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SEA_SURFACE_TEMPERATURE_NIGHT]  = Texture::fromFile("./Resources/Nasa Earth Data/Sea Surface Temperature Night CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::LAND_SURFACE_TEMPERATURE_DAY]   = Texture::fromFile("./Resources/Nasa Earth Data/Land Surface Temperature CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::LAND_SURFACE_TEMPERATURE_NIGHT] = Texture::fromFile("./Resources/Nasa Earth Data/Land Surface Temperature Night CAF.png", Texture_Format::MONO_FLOAT);
	
	textures[Texture_Field::HUMIDITY]                = Texture::fromFile("./Resources/Nasa Earth Data/MODIS/Humidity After Moist CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::WATER_VAPOR]             = Texture::fromFile("./Resources/Nasa Earth Data/Water Vapor CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::CLOUD_COVERAGE]          = Texture::fromFile("./Resources/Nasa Earth Data/Cloud Fraction.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::CLOUD_WATER_CONTENT]     = Texture::fromFile("./Resources/Nasa Earth Data/Cloud Water Content CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::CLOUD_PARTICLE_RADIUS]   = Texture::fromFile("./Resources/Nasa Earth Data/Cloud Particle Radius CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::CLOUD_OPTICAL_THICKNESS] = Texture::fromFile("./Resources/Nasa Earth Data/Cloud Optical Thickness CAF.png", Texture_Format::MONO_FLOAT);
	
	textures[Texture_Field::OZONE]                         = Texture::fromFile("./Resources/Nasa Earth Data/Ozone CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::UV_INDEX]                      = Texture::fromFile("./Resources/Nasa Earth Data/UV Index.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::NET_RADIATION]                 = Texture::fromFile("./Resources/Nasa Earth Data/Net Radiation.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SOLAR_INSOLATION]              = Texture::fromFile("./Resources/Nasa Earth Data/Solar Insolation.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::OUTGOING_LONGWAVE_RADIATION]   = Texture::fromFile("./Resources/Nasa Earth Data/Outgoing Longwave Radiation.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::REFLECTED_SHORTWAVE_RADIATION] = Texture::fromFile("./Resources/Nasa Earth Data/Reflected Shortwave Radiation.png", Texture_Format::MONO_FLOAT);
}

void Kernel::preInit(const unordered_map<string, float>& params_float, const unordered_map<string, bool>& params_bool,const unordered_map<string, int>& params_int) {
	this->params_float = params_float;
	this->params_bool = params_bool;
	this->params_int = params_int;

	PARTICLE_RADIUS  = 0.025f;
	PARTICLE_COUNT   = params_int.at("PARTICLE_COUNT");
	MAX_OCTREE_DEPTH = params_int.at("MAX_OCTREE_DEPTH");
	POLE_BIAS        = params_float.at("POLE_BIAS");
	POLE_BIAS_POWER  = params_float.at("POLE_BIAS_POWER");
	POLE_GEOLOCATION = vec2(params_float.at("POLE_GEOLOCATION.x"), params_float.at("POLE_GEOLOCATION.y"));
	DT               = 0.016f;
	RUNFRAME         = 0;
	SAMPLES          = 5;
	SDT              = 0.016f / u_to_f(SAMPLES);

	preInitParticles();
	initBvh();
}

void Kernel::preInitParticles() {
	const vec1 radius = 6.371f + PARTICLE_RADIUS;

	particles.clear();
	for (uint i = 0; i < PARTICLE_COUNT; i++) {
		CPU_Particle particle = CPU_Particle();
		const vec1 normalized_i = i / (vec1)(PARTICLE_COUNT - 1);
		const vec1 biased_i = (1.0f - POLE_BIAS) * normalized_i + POLE_BIAS * pow(normalized_i, POLE_BIAS_POWER);

		const vec1 theta = acos(1.0f - 2.0f * biased_i);
		const vec1 phi = vec1(i) * (glm::pi<vec1>() * (3.0f - sqrt(5.0f)));

		const vec1 x = radius * sin(theta) * cos(phi);
		const vec1 y = radius * cos(theta);
		const vec1 z = radius * sin(theta) * sin(phi);

		particle.position = rotateGeoloc(vec3(x, y, z), POLE_GEOLOCATION);

		traceProperties(&particle);
		particles.push_back(particle);
	}
}

void Kernel::init() {
	textures.clear();
	initParticles();
	time = 0.0f;
	frame_count = 0;
}

void Kernel::initParticles() {
	const int NUM_NEIGHBORS = 3;

	for (uint i = 0; i < PARTICLE_COUNT; i++) {
		CPU_Particle& particle = particles[i];

		vector<CPU_Neighbor> neighbors;

		for (uint j = 0; j < PARTICLE_COUNT; j++) {
			if (i != j) {
				const vec1 distSq = glm::distance2(particle.position, particles[j].position);
				neighbors.push_back(CPU_Neighbor(distSq, &particles[j]));
			}
		}

		sort(neighbors.begin(), neighbors.end(), [](const CPU_Neighbor& a, const CPU_Neighbor& b) {
			return a.distance < b.distance;
		});

		particle.smoothing_radius = neighbors[2].distance * 2.0f;
		for (uint k = 0; k < NUM_NEIGHBORS; k++) {
			particle.neighbors.push_back(neighbors[k]);
		}
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

	time += DT;

	frame_count++;
}

void Kernel::traceProperties(CPU_Particle* particle) const {
	const vec3 ray_direction = glm::normalize(vec3(0) - particle->position);

	const vec1 a = glm::dot(ray_direction, ray_direction);
	const vec1 b = 2.0f * dot(ray_direction, particle->position);
	const vec1 c = dot(particle->position, particle->position) - 40.589641f; // Earth Radius ^2
	const vec1 delta = b * b - 4.0f * a * c;
	if (delta < 0.0f) {
		return;
	}

	const vec3 intersectionPoint = particle->position + ((-b - sqrt(delta)) / (2.0f * a)) * ray_direction;
	const vec1 axialTilt = -glm::radians(params_float.at("EARTH_TILT"));
	const mat3 tiltRotation = mat3(
		vec3(cos(axialTilt), -sin(axialTilt), 0),
		vec3(sin(axialTilt), cos(axialTilt), 0),
		vec3(0, 0, 1));
	const vec3 normal = tiltRotation * glm::normalize(intersectionPoint);

	const vec1 theta = acos(normal.y);
	const vec1 phi = glm::atan(normal.z, normal.x);

	const vec2 uv = vec2(1.0 - ((phi + PI) / TWO_PI), (theta) / PI);

	const vec1 topography_sample = textures.at(Texture_Field::TOPOGRAPHY                    ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 bathymetry_sample = textures.at(Texture_Field::BATHYMETRY                    ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 pressure_sample   = textures.at(Texture_Field::SURFACE_PRESSURE              ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 sst_sample        = textures.at(Texture_Field::SEA_SURFACE_TEMPERATURE_DAY   ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 sst_night_sample  = textures.at(Texture_Field::SEA_SURFACE_TEMPERATURE_NIGHT ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 lst_sample        = textures.at(Texture_Field::LAND_SURFACE_TEMPERATURE_DAY  ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 lst_night_sample  = textures.at(Texture_Field::LAND_SURFACE_TEMPERATURE_NIGHT).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);

	const vec1 humidity_sample                = textures.at(Texture_Field::HUMIDITY                  ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 water_vapor_sample             = textures.at(Texture_Field::WATER_VAPOR               ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 cloud_coverage_sample          = textures.at(Texture_Field::CLOUD_COVERAGE            ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 cloud_water_content_sample     = textures.at(Texture_Field::CLOUD_WATER_CONTENT       ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 cloud_particle_radius_sample   = textures.at(Texture_Field::CLOUD_PARTICLE_RADIUS     ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 cloud_optical_thickness_sample = textures.at(Texture_Field::CLOUD_OPTICAL_THICKNESS   ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);

	const vec1 ozone_sample                         = textures.at(Texture_Field::OZONE                        ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 uv_index_sample                      = textures.at(Texture_Field::UV_INDEX                     ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 net_radiation_sample                 = textures.at(Texture_Field::NET_RADIATION                ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 solar_insolation_sample              = textures.at(Texture_Field::SOLAR_INSOLATION             ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 outgoiing_longwave_radiation_sample  = textures.at(Texture_Field::OUTGOING_LONGWAVE_RADIATION  ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 reflected_shortwave_radiation_sample = textures.at(Texture_Field::REFLECTED_SHORTWAVE_RADIATION).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);

	const vec1 topography = lut(Texture_Field::TOPOGRAPHY, topography_sample);
	const vec1 bathymetry = lut(Texture_Field::BATHYMETRY, bathymetry_sample);
	particle->pressure    = lut(Texture_Field::SURFACE_PRESSURE, pressure_sample);
	const vec1 sst        = lut(Texture_Field::SEA_SURFACE_TEMPERATURE_DAY, sst_sample);
	const vec1 sst_night  = lut(Texture_Field::SEA_SURFACE_TEMPERATURE_NIGHT, sst_night_sample);
	const vec1 lst        = lut(Texture_Field::LAND_SURFACE_TEMPERATURE_DAY, lst_sample);
	const vec1 lst_night  = lut(Texture_Field::LAND_SURFACE_TEMPERATURE_NIGHT, lst_night_sample);
	if (topography == -1.0f) { // Is at sea
		particle->day_temperature = sst;
		particle->night_temperature = sst_night;
		particle->on_water = true;
		particle->height = bathymetry;
	}
	else { // Is on Land
		particle->day_temperature = lst;
		particle->night_temperature = lst_night;
		particle->on_water = false;
		particle->height = topography;
	}

	particle->humidity                = lut(Texture_Field::HUMIDITY, humidity_sample);
	particle->water_vapor             = lut(Texture_Field::WATER_VAPOR, water_vapor_sample);
	particle->cloud_coverage          = lut(Texture_Field::CLOUD_COVERAGE, cloud_coverage_sample);
	particle->cloud_water_content     = lut(Texture_Field::CLOUD_WATER_CONTENT, cloud_water_content_sample);
	particle->cloud_particle_radius   = lut(Texture_Field::CLOUD_PARTICLE_RADIUS, cloud_particle_radius_sample);
	particle->cloud_optical_thickness = lut(Texture_Field::CLOUD_OPTICAL_THICKNESS, cloud_optical_thickness_sample);

	particle->ozone                         = lut(Texture_Field::OZONE, ozone_sample);
	particle->uv_index                      = lut(Texture_Field::UV_INDEX, uv_index_sample);
	particle->net_radiation                 = lut(Texture_Field::NET_RADIATION, net_radiation_sample);
	particle->solar_insolation              = lut(Texture_Field::SOLAR_INSOLATION, solar_insolation_sample);
	particle->outgoing_longwave_radiation   = lut(Texture_Field::OUTGOING_LONGWAVE_RADIATION, outgoiing_longwave_radiation_sample);
	particle->reflected_shortwave_radiation = lut(Texture_Field::REFLECTED_SHORTWAVE_RADIATION, reflected_shortwave_radiation_sample);
}

vec3 Kernel::rotateGeoloc(const vec3& point, const vec2& geoloc) const {
	const vec1 phi = glm::radians(geoloc.x - 90.0f);
	const vec1 theta = glm::radians(geoloc.y + 90.0f);
	const vec1 axialTilt = -glm::radians(params_float.at("EARTH_TILT"));

	const quat tiltRotation = glm::angleAxis(axialTilt, glm::vec3(0, 0, 1));
	const quat latRotation  = glm::angleAxis(phi, glm::vec3(1, 0, 0));
	const quat lonRotation  = glm::angleAxis(theta, glm::vec3(0, 1, 0));
	const quat combinedRotation = tiltRotation * lonRotation * latRotation;

	return combinedRotation * point;
}

vec3 Kernel::sunDir() const {
	const vec2 time = vec2(0, params_float.at("DATE_TIME") * TWO_PI);
	const vec2 c = cos(time);
	const vec2 s = sin(time);

	const mat3 rot = mat3(
		c.y      ,  0.0, -s.y,
		s.y * s.x,  c.x,  c.y * s.x,
		s.y * c.x, -s.x,  c.y * c.x
	);
	return glm::normalize(vec3(-1,0,0) * rot);
}

vec1 dateToFloat(const int& month, const int& day) {
	const array<int, 12> daysInMonth = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	int dayOfYear = 0;
	for (int i = 0; i < month - 1; ++i) {
		dayOfYear += daysInMonth[i];
	}
	dayOfYear += day;

	const int totalDaysInYear = 365;
	vec1 adjustedValue = (355 - dayOfYear + 1) / static_cast<vec1>(totalDaysInYear); // Start in solstice december 21

	if (adjustedValue < 0) {
		adjustedValue = 1.0f + adjustedValue;
	}
	return adjustedValue;
}