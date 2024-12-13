#include "Particle.hpp"

CPU_Probe_Data::CPU_Probe_Data() {
	position = dvec3(0);
	wind_vector  = dvec3(0);
	wind_u = 0;
	wind_v = 0;
	wind_quaternion = glm::normalize(dquat(randD(), randD(), randD(), randD()));
	surface_area = 1;

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
	emissivity = 0;
	sun_intensity = 0;
	net_radiation = 0;
	solar_insolation = 0;
	solar_irradiance = 0;
	outgoing_longwave_radiation = 0;
	reflected_shortwave_radiation = 0;
	on_water = false;
}

CPU_Probe::CPU_Probe() :
	new_data(CPU_Probe_Data()),
	data(CPU_Probe_Data()),
	sph(CPU_Probe_Data())
{
	gen_index = 0;

	transformed_position = dvec3(0);
	smoothing_radius = 0.0f;
}

GPU_Probe::GPU_Probe() {
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

	sph_pressure = 0;
	sph_wind_vector = vec3(0);
	sph_temperature = 0;
}

GPU_Probe::GPU_Probe(const CPU_Probe* particle) {
	gen_index = particle->gen_index;
	smoothing_radius = d_to_f(particle->smoothing_radius);

	position      = d_to_f(particle->transformed_position);
	wind_vector   = d_to_f(particle->data.wind_vector);
	sun_intensity = d_to_f(particle->data.sun_intensity);

	wind_u = d_to_f(particle->data.wind_u);
	wind_v = d_to_f(particle->data.wind_v);

	height            = d_to_f(particle->data.height);
	pressure          = d_to_f(particle->data.pressure);
	temperature       = d_to_f(particle->data.temperature);
	day_temperature   = d_to_f(particle->data.day_temperature);
	night_temperature = d_to_f(particle->data.night_temperature);

	humidity                = d_to_f(particle->data.humidity);
	water_vapor             = d_to_f(particle->data.water_vapor);
	cloud_coverage          = d_to_f(particle->data.cloud_coverage);
	cloud_water_content     = d_to_f(particle->data.cloud_water_content);
	cloud_particle_radius   = d_to_f(particle->data.cloud_particle_radius);
	cloud_optical_thickness = d_to_f(particle->data.cloud_optical_thickness);

	ozone                         = d_to_f(particle->data.ozone);
	albedo                        = d_to_f(particle->data.albedo);
	uv_index                      = d_to_f(particle->data.uv_index);
	net_radiation                 = d_to_f(particle->data.net_radiation);
	solar_insolation              = d_to_f(particle->data.solar_insolation);
	outgoing_longwave_radiation   = d_to_f(particle->data.outgoing_longwave_radiation);
	reflected_shortwave_radiation = d_to_f(particle->data.reflected_shortwave_radiation);

	sph_pressure    = d_to_f(particle->sph.pressure);
	sph_wind_vector = d_to_f(particle->sph.wind_vector);
	sph_temperature = d_to_f(particle->sph.temperature);
}

CPU_Neighbor::CPU_Neighbor(const dvec1& distance, CPU_Probe* neighbor) :
	distance(distance),
	probe(neighbor)
{}

CPU_Particle::CPU_Particle() :
	transformed_position(dvec3(0)),
	position(dvec3(0)),
	wind_speed(glm::normalize(dquat(randD(), randD(), randD(), randD()))),
	rotation(dquat(1,0,0,0)),
	probe(nullptr)
{}

GPU_Particle::GPU_Particle(const CPU_Particle* particle) :
	position(d_to_f(particle->transformed_position), 0.0f)
{}

GPU_Particle::GPU_Particle(const Compute_Particle& particle) :
	position(particle.position, 0.0f)
{}

Compute_Probe::Compute_Probe(const CPU_Probe& probe, CPU_Probe* first) {
	position = vec4(d_to_f(probe.transformed_position), 0.0f);
	wind_speed = vec4(d_to_f(probe.data.wind_quaternion.w), d_to_f(probe.data.wind_quaternion.x), d_to_f(probe.data.wind_quaternion.y), d_to_f(probe.data.wind_quaternion.z));
	neighbors = uvec3(0);

	neighbors[0] = (int)(probe.neighbors[0].probe - first);
	neighbors[1] = (int)(probe.neighbors[1].probe - first);
	neighbors[2] = (int)(probe.neighbors[2].probe - first);
}

Compute_Particle::Compute_Particle(const CPU_Particle& particle, CPU_Probe* first) {
	position = particle.transformed_position;
	closest = (int)(particle.probe - first);
	rotation = vec4(d_to_f(particle.wind_speed.w), d_to_f(particle.wind_speed.x), d_to_f(particle.wind_speed.y), d_to_f(particle.wind_speed.z));
}