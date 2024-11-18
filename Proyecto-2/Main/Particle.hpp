#pragma once

#include "Shared.hpp"

struct CPU_Neighbor;
struct CPU_Probe;
struct GPU_Probe;
struct CPU_Particle;
struct GPU_Particle;
struct Compute_Probe;
struct Compute_Particle;

struct CPU_Probe_Data {
	dvec3 position; // mm (mega) meters
	dvec3 wind_vector; // m/s
	dvec1 wind_u;
	dvec1 wind_v;
	dquat wind_quaternion;
	dvec1 surface_area; // mm (mega) meters

	dvec1 height; // m
	dvec1 pressure; // hPa
	dvec1 temperature; // K
	dvec1 day_temperature; // K
	dvec1 night_temperature; // K

	dvec1 humidity;
	dvec1 water_vapor; // cm
	dvec1 cloud_coverage; // %
	dvec1 cloud_water_content; // g/m^2
	dvec1 cloud_particle_radius; // μm
	dvec1 cloud_optical_thickness; // 0-50%

	dvec1 ozone; // Dobson
	dvec1 albedo; // %
	dvec1 uv_index; // 0-16
	dvec1 emissivity; // %
	dvec1 sun_intensity; // %
	dvec1 net_radiation; // W/m^2
	dvec1 solar_insolation; // W/m^2
	dvec1 solar_irradiance; // W/m^2
	dvec1 outgoing_longwave_radiation; // W/m^2
	dvec1 reflected_shortwave_radiation; // W/m^2

	bool on_water;

	CPU_Probe_Data();
};

struct CPU_Probe {
	uint gen_index;
	CPU_Probe_Data new_data;
	CPU_Probe_Data data;
	CPU_Probe_Data sph;
	dvec3 transformed_position;

	dvec1 smoothing_radius;
	vector<CPU_Neighbor> neighbors;

	CPU_Probe();
};

struct CPU_Neighbor {
	dvec1 distance;
	CPU_Probe* probe;

	CPU_Neighbor(const dvec1& distance, CPU_Probe* neighbor);
};

struct CPU_Particle {
	dquat rotation;
	dquat wind_speed;
	dvec3 position;
	dvec3 transformed_position;
	CPU_Probe* probe;

	CPU_Particle();
};

struct alignas(16) GPU_Particle {
	vec4 position;

	GPU_Particle(const CPU_Particle* particle);
	GPU_Particle(const Compute_Particle& particle);
};

struct alignas(16) GPU_Probe {
	vec3 position;
	vec1 height;

	vec3 wind_vector;
	vec1 sun_intensity;

	vec1 wind_u;
	vec1 wind_v;
	vec2 padding = vec2(0);

	vec1 pressure;
	vec1 temperature;
	vec1 day_temperature;
	vec1 night_temperature;

	vec1 humidity;
	vec1 water_vapor;
	vec1 cloud_coverage;
	vec1 cloud_water_content;

	vec1 cloud_particle_radius;
	vec1 cloud_optical_thickness;
	vec1 ozone;
	vec1 albedo;

	vec1 uv_index;
	vec1 net_radiation;
	vec1 solar_insolation;
	vec1 outgoing_longwave_radiation;

	vec1 reflected_shortwave_radiation;
	uint gen_index;
	vec1 smoothing_radius;

	vec1 sph_pressure;

	vec3 sph_wind_vector;
	vec1 sph_temperature;

	GPU_Probe();
	GPU_Probe(const CPU_Probe* probe);
};

struct alignas(16) Compute_Probe {
	vec4  position;
	vec4  wind_speed;
	uvec3 neighbors;
	vec1  padding = 0.0f;

	Compute_Probe(const CPU_Probe& probe, CPU_Probe* first);
};

struct alignas(16) Compute_Particle {
	vec3 position;
	uint closest;
	vec4 rotation;

	Compute_Particle(const CPU_Particle& particle, CPU_Probe* first);
};