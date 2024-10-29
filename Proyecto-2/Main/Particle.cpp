#include "Particle.hpp"

CPU_Particle_Data::CPU_Particle_Data() {
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
}

CPU_Particle::CPU_Particle() :
	data(CPU_Particle_Data()),
	sph(CPU_Particle_Data())
{
	gen_index = 0;

	transformed_position = vec3(0);
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
	gen_index = 0;
	smoothing_radius = 0;
}

GPU_Particle::GPU_Particle(const CPU_Particle& particle) {
	gen_index = particle.gen_index;
	smoothing_radius = particle.smoothing_radius;

	position = particle.transformed_position;
	wind_vector = particle.data.wind_vector;
	sun_intensity = particle.data.sun_intensity;

	height = particle.data.height;
	pressure = particle.data.pressure;
	temperature = particle.data.temperature;
	day_temperature = particle.data.day_temperature;
	night_temperature = particle.data.night_temperature;
	humidity = particle.data.humidity;
	water_vapor = particle.data.water_vapor;
	cloud_coverage = particle.data.cloud_coverage;
	cloud_water_content = particle.data.cloud_water_content;
	cloud_particle_radius = particle.data.cloud_particle_radius;
	cloud_optical_thickness = particle.data.cloud_optical_thickness;
	ozone = particle.data.ozone;
	albedo = particle.data.albedo;
	uv_index = particle.data.uv_index;
	net_radiation = particle.data.net_radiation;
	solar_insolation = particle.data.solar_insolation;
	outgoing_longwave_radiation = particle.data.outgoing_longwave_radiation;
	reflected_shortwave_radiation = particle.data.reflected_shortwave_radiation;
}

CPU_Neighbor::CPU_Neighbor(const vec1& distance, CPU_Particle* neighbor) :
	distance(distance),
	neighbor(neighbor)
{}