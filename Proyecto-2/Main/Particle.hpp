#pragma once

#include "Shared.hpp"

struct CPU_Particle;
struct CPU_Neighbor;

struct CPU_Particle_Data {
	vec3 position; // mm (mega) meters
	vec3 wind_vector; // m/s
	vec1 sun_intensity; // %

	vec1 height; // m
	vec1 pressure; // hPa
	vec1 temperature; // C
	vec1 day_temperature; // C
	vec1 night_temperature; // C

	vec1 humidity;
	vec1 water_vapor; // cm
	vec1 cloud_coverage; // %
	vec1 cloud_water_content; // g/m^2
	vec1 cloud_particle_radius; // μm
	vec1 cloud_optical_thickness; // 0-50%

	vec1 ozone; // Dobson
	vec1 albedo; // %
	vec1 uv_index;
	vec1 net_radiation; // W/m^2
	vec1 solar_insolation; // W/m^2
	vec1 outgoing_longwave_radiation; // W/m^2
	vec1 reflected_shortwave_radiation; // W/m^2

	bool on_water;

	CPU_Particle_Data();
};

struct CPU_Particle {
	uint gen_index;
	CPU_Particle_Data new_data;
	CPU_Particle_Data data;
	CPU_Particle_Data sph;
	vec3 transformed_position;

	vec1 smoothing_radius;
	vector<CPU_Neighbor> neighbors;

	CPU_Particle();
};

struct CPU_Neighbor {
	vec1 distance;
	CPU_Particle* neighbor;

	CPU_Neighbor(const vec1& distance, CPU_Particle* neighbor);
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
	vec1 pad_c = 0;

	GPU_Particle();
	GPU_Particle(const CPU_Particle& particle);
};