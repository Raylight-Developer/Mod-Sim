#include "Particle.hpp"

CPU_Particle::CPU_Particle() {
	position = vec3(0);
	wind_vector  = vec3(0);
	sun_intensity = 0;

	height = 0;
	pressure = 0;
	temperature = 0;
	day_temperature = 0;
	night_temperature = 0;
	humidity = 0;
	water_vapor = 0;
	cloud_coverage = 0;
	cloud_water_content = 0;
	cloud_particle_radius = 0;
	cloud_optical_thickness = 0;
	ozone = 0;
	albedo = 0;
	uv_index = 0;
	net_radiation = 0;
	solar_insolation = 0;
	outgoing_longwave_radiation = 0;
	reflected_shortwave_radiation = 0;
	on_water = false;

	smoothing_radius = 0.0f;
}

GPU_Particle::GPU_Particle() {
	position = vec3(0);
	wind_vector  = vec3(0);
	sun_intensity = 0;

	height = 0;
	pressure = 0;
	temperature = 0;
	day_temperature = 0;
	night_temperature = 0;
	humidity = 0;
	water_vapor = 0;
	cloud_coverage = 0;
	cloud_water_content = 0;
	cloud_particle_radius = 0;
	cloud_optical_thickness = 0;
	ozone = 0;
	albedo = 0;
	uv_index = 0;
	net_radiation = 0;
	solar_insolation = 0;
	outgoing_longwave_radiation = 0;
	reflected_shortwave_radiation = 0;
}

GPU_Particle::GPU_Particle(const CPU_Particle& particle) {
	position = particle.position;
	wind_vector = particle.wind_vector;
	sun_intensity = particle.sun_intensity;

	height = particle.height;
	pressure = particle.pressure;
	temperature = particle.temperature;
	day_temperature = particle.day_temperature;
	night_temperature = particle.night_temperature;
	humidity = particle.humidity;
	water_vapor = particle.water_vapor;
	cloud_coverage = particle.cloud_coverage;
	cloud_water_content = particle.cloud_water_content;
	cloud_particle_radius = particle.cloud_particle_radius;
	cloud_optical_thickness = particle.cloud_optical_thickness;
	ozone = particle.ozone;
	albedo = particle.albedo;
	uv_index = particle.uv_index;
	net_radiation = particle.net_radiation;
	solar_insolation = particle.solar_insolation;
	outgoing_longwave_radiation = particle.outgoing_longwave_radiation;
	reflected_shortwave_radiation = particle.reflected_shortwave_radiation;
}

CPU_Neighbor::CPU_Neighbor(const vec1& distance, CPU_Particle* neighbor) :
	distance(distance),
	neighbor(neighbor)
{}