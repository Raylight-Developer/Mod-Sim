#pragma once

#include "Shared.hpp"

struct CPU_Particle;
struct CPU_Neighbor;

struct CPU_Particle_Data {
	dvec3 position; // mm (mega) meters
	dvec3 wind_vector; // m/s
	dvec3 pressure_gradient;
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

	CPU_Particle_Data();
};

struct CPU_Particle {
	uint gen_index;
	CPU_Particle_Data new_data;
	CPU_Particle_Data data;
	CPU_Particle_Data sph;
	dvec3 transformed_position;

	dvec1 smoothing_radius;
	vector<CPU_Neighbor> neighbors;

	CPU_Particle();
};

struct CPU_Neighbor {
	dvec1 distance;
	CPU_Particle* neighbor;

	CPU_Neighbor(const dvec1& distance, CPU_Particle* neighbor);
};

struct alignas(16) GPU_Particle {
	vec3 position;
	vec1 height;

	vec3 wind_vector;
	vec1 sun_intensity;

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

	GPU_Particle();
	GPU_Particle(const CPU_Particle& particle);
};