#include "Kernel.hpp"

#include "Lut.hpp"

#define STEFAN_BOLZMANN 5.67e-8
#define AIR_SPECIFIC_HEAT_CAPACITY 1005.0 // J/kg°C
#define AIR_DENSITY                1.225 // kg/m³
#define DRAG_COEFFICIENT           0.1

#define CORIOLIS           vec3(15.0, 0, 0)

#pragma optimize("O3", on)
Kernel::Kernel() {
	PARTICLE_RADIUS           = 0.025f;
	PARTICLE_COUNT            = 8192;
	PARTICLE_MAX_OCTREE_DEPTH = 2;
	PARTICLE_POLE_BIAS        = 0.0;
	PARTICLE_POLE_BIAS_POWER  = 1.0;
	PARTICLE_POLE_GEOLOCATION = dvec2(25.0, 90.0);

	PROBE_RADIUS           = 0.05f;
	PROBE_COUNT            = 8192;
	PROBE_MAX_OCTREE_DEPTH = 3;
	PROBE_POLE_BIAS        = 0.0;//0.975;
	PROBE_POLE_BIAS_POWER  = 1.0;// 5.0;
	PROBE_POLE_GEOLOCATION = dvec2(25.0, 90.0);
	BVH_SPH                = false;

	EARTH_TILT      = 23.5;
	CALENDAR_DAY    = 21;
	CALENDAR_MONTH  = 12;
	CALENDAR_HOUR   = 21;
	CALENDAR_MINUTE = 0;
	YEAR_TIME       = 0;
	DAY_TIME        = 0;
	DAY             = 0;
	TIME_SCALE      = 1.0;
	DT              = 0;
	RUNFRAME        = 0;
	SUB_SAMPLES     = 2;
	SDT             = 0;
	sun_dir         = dvec3(0, 0, 1);
	calculateDateTime();

#ifdef NDEBUG
	textures[Texture_Field::TOPOGRAPHY]                     = Texture::fromFile("./Resources/Data/Topography.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::BATHYMETRY]                     = Texture::fromFile("./Resources/Data/Bathymetry.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SURFACE_PRESSURE]               = Texture::fromFile("./Resources/Data/Pressure CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SEA_SURFACE_TEMPERATURE_DAY]    = Texture::fromFile("./Resources/Data/Sea Surface Temperature CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SEA_SURFACE_TEMPERATURE_NIGHT]  = Texture::fromFile("./Resources/Data/Sea Surface Temperature Night CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::LAND_SURFACE_TEMPERATURE_DAY]   = Texture::fromFile("./Resources/Data/Land Surface Temperature CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::LAND_SURFACE_TEMPERATURE_NIGHT] = Texture::fromFile("./Resources/Data/Land Surface Temperature Night CAF.png", Texture_Format::MONO_FLOAT);
	
	textures[Texture_Field::HUMIDITY]                = Texture::fromFile("./Resources/Data/Humidity CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::WATER_VAPOR]             = Texture::fromFile("./Resources/Data/Water Vapor CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::CLOUD_COVERAGE]          = Texture::fromFile("./Resources/Data/Cloud Fraction.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::CLOUD_WATER_CONTENT]     = Texture::fromFile("./Resources/Data/Cloud Water Content CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::CLOUD_PARTICLE_RADIUS]   = Texture::fromFile("./Resources/Data/Cloud Particle Radius CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::CLOUD_OPTICAL_THICKNESS] = Texture::fromFile("./Resources/Data/Cloud Optical Thickness CAF.png", Texture_Format::MONO_FLOAT);
	
	textures[Texture_Field::OZONE]                         = Texture::fromFile("./Resources/Data/Ozone CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::ALBEDO]                        = Texture::fromFile("./Resources/Data/Albedo CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::UV_INDEX]                      = Texture::fromFile("./Resources/Data/UV Index.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::NET_RADIATION]                 = Texture::fromFile("./Resources/Data/Net Radiation.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SOLAR_INSOLATION]              = Texture::fromFile("./Resources/Data/Solar Insolation.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::OUTGOING_LONGWAVE_RADIATION]   = Texture::fromFile("./Resources/Data/Outgoing Longwave Radiation.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::REFLECTED_SHORTWAVE_RADIATION] = Texture::fromFile("./Resources/Data/Reflected Shortwave Radiation.png", Texture_Format::MONO_FLOAT);

	textures[Texture_Field::WIND_VECTOR] = Texture::fromFile("./Resources/Data/Wind.png", Texture_Format::RGBA_8);
#else
	textures[Texture_Field::TOPOGRAPHY]                     = Texture::fromFile("./Resources/Data/Topography LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::BATHYMETRY]                     = Texture::fromFile("./Resources/Data/Bathymetry LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SURFACE_PRESSURE]               = Texture::fromFile("./Resources/Data/Pressure LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SEA_SURFACE_TEMPERATURE_DAY]    = Texture::fromFile("./Resources/Data/Sea Surface Temperature LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SEA_SURFACE_TEMPERATURE_NIGHT]  = Texture::fromFile("./Resources/Data/Sea Surface Temperature Night LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::LAND_SURFACE_TEMPERATURE_DAY]   = Texture::fromFile("./Resources/Data/Land Surface Temperature LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::LAND_SURFACE_TEMPERATURE_NIGHT] = Texture::fromFile("./Resources/Data/Land Surface Temperature Night LR.png", Texture_Format::MONO_FLOAT);

	textures[Texture_Field::HUMIDITY]                = Texture::fromFile("./Resources/Data/Humidity LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::WATER_VAPOR]             = Texture::fromFile("./Resources/Data/Water Vapor LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::CLOUD_COVERAGE]          = Texture::fromFile("./Resources/Data/Cloud Fraction LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::CLOUD_WATER_CONTENT]     = Texture::fromFile("./Resources/Data/Cloud Water Content LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::CLOUD_PARTICLE_RADIUS]   = Texture::fromFile("./Resources/Data/Cloud Particle Radius LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::CLOUD_OPTICAL_THICKNESS] = Texture::fromFile("./Resources/Data/Cloud Optical Thickness LR.png", Texture_Format::MONO_FLOAT);

	textures[Texture_Field::OZONE]                         = Texture::fromFile("./Resources/Data/Ozone LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::ALBEDO]                        = Texture::fromFile("./Resources/Data/Albedo LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::UV_INDEX]                      = Texture::fromFile("./Resources/Data/UV Index LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::NET_RADIATION]                 = Texture::fromFile("./Resources/Data/Net Radiation LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SOLAR_INSOLATION]              = Texture::fromFile("./Resources/Data/Solar Insolation LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::OUTGOING_LONGWAVE_RADIATION]   = Texture::fromFile("./Resources/Data/Outgoing Longwave Radiation LR.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::REFLECTED_SHORTWAVE_RADIATION] = Texture::fromFile("./Resources/Data/Reflected Shortwave Radiation LR.png", Texture_Format::MONO_FLOAT);
#endif
}
#pragma optimize("", on)

void Kernel::updateGPUProbes() {
	const Builder bvh_build = BVH_SPH ? Builder(probes, -1.0f, 1): Builder(probes, d_to_f(PROBE_RADIUS), PROBE_MAX_OCTREE_DEPTH);
	probe_nodes = bvh_build.nodes;

	gpu_probes.clear();
	for (const CPU_Probe& probe : bvh_build.probes) {
		gpu_probes.push_back(GPU_Probe(probe));
	}
}

void Kernel::buildProbes() {
	cout << "Rebuild Probes" << endl;
	const dvec1 radius = 6.371 + PROBE_RADIUS;
	sun_dir = sunDir();

	probes.clear();
	for (uint i = 0; i < PROBE_COUNT; i++) {
		CPU_Probe probe = CPU_Probe();
		const dvec1 normalized_i = i / (dvec1)(PROBE_COUNT - 1);
		const dvec1 biased_i = (1.0 - PROBE_POLE_BIAS) * normalized_i + PROBE_POLE_BIAS * pow(normalized_i, PROBE_POLE_BIAS_POWER);

		const dvec1 theta = acos(1.0 - 2.0 * biased_i);
		const dvec1 phi = dvec1(i) * (glm::pi<dvec1>() * (3.0 - sqrt(5.0)));

		const dvec1 x = radius * sin(theta) * cos(phi);
		const dvec1 y = radius * cos(theta);
		const dvec1 z = radius * sin(theta) * sin(phi);

		probe.data.position = rotateGeoloc(dvec3(x, y, z), PROBE_POLE_GEOLOCATION);
		updateProbePosition(&probe);

		traceInitProperties(&probe);
		probe.gen_index = i;
		probes.push_back(probe);
	}
}

void Kernel::updateGPUParticles() {
	const Particle_Builder bvh_build = Particle_Builder(particles, d_to_f(PARTICLE_RADIUS), PROBE_MAX_OCTREE_DEPTH);
	particle_nodes = bvh_build.nodes;

	gpu_particles.clear();
	for (const CPU_Particle& particle : bvh_build.particles) {
		gpu_particles.push_back(GPU_Particle(particle));
	}
}

void Kernel::buildParticles() {
	cout << "Rebuild Particles" << endl;
	const dvec1 radius = 6.371 + PROBE_RADIUS * 2.0 + PARTICLE_RADIUS;
	sun_dir = sunDir();

	particles.clear();
	for (uint i = 0; i < PARTICLE_COUNT; i++) {
		CPU_Particle particle = CPU_Particle();
		const dvec1 normalized_i = i / (dvec1)(PARTICLE_COUNT - 1);
		const dvec1 biased_i = (1.0 - PARTICLE_POLE_BIAS) * normalized_i + PARTICLE_POLE_BIAS * pow(normalized_i, PARTICLE_POLE_BIAS_POWER);

		const dvec1 theta = acos(1.0 - 2.0 * biased_i);
		const dvec1 phi = dvec1(i) * (glm::pi<dvec1>() * (3.0 - sqrt(5.0)));

		const dvec1 x = radius * sin(theta) * cos(phi);
		const dvec1 y = radius * cos(theta);
		const dvec1 z = radius * sin(theta) * sin(phi);

		particle.position = rotateGeoloc(dvec3(x, y, z), PARTICLE_POLE_GEOLOCATION);
		updateParticlePosition(&particle);
		particles.push_back(particle);
	}
}

void Kernel::lock() {
	textures.clear();
	lockProbes();
	lockParticles();
}

void Kernel::lockProbes() {
	const int NUM_NEIGHBORS = 3;

	int i = 0;
	int i_size = u_to_i(PROBE_COUNT);
	#pragma omp parallel for private(i) num_threads(12)
	for (i = 0; i < i_size; i++) {
		CPU_Probe& probe = probes[i];

		vector<CPU_Neighbor> neighbors;

		for (uint j = 0; j < PROBE_COUNT; j++) {
			if (i != j) {
				const dvec1 dist = glm::distance(probe.data.position, probes[j].data.position);
				neighbors.push_back(CPU_Neighbor(dist, &probes[j]));
			}
		}

		sort(neighbors.begin(), neighbors.end(), [](const CPU_Neighbor& a, const CPU_Neighbor& b) {
			return a.distance < b.distance;
		});

		probe.smoothing_radius = neighbors[NUM_NEIGHBORS].distance * 1.25;
		for (uint k = 0; k < NUM_NEIGHBORS; k++) {
			probe.neighbors.push_back(neighbors[k]);
		}

		for (CPU_Neighbor& neighbor : probe.neighbors) {
			const dvec1 smoothing_kernel = pow(glm::max(0.0, probe.smoothing_radius - neighbor.distance), 3.0);

			const dvec3 direction = neighbor.probe->data.position - probe.data.position;

			const dvec3 unitDirection = direction / neighbor.distance;
			const dvec1 pressureDifference = (probe.data.pressure - neighbor.probe->data.pressure) * 10.0;
			probe.data.wind_vector += unitDirection * (pressureDifference) * smoothing_kernel * 10.0;
		}

		//if ((i % (PROBE_COUNT / 5)) == 0) {
		//	#pragma omp critical
		//	cout << "Lock Probes: " << f_to_u(round(u_to_f(i) / u_to_f(PROBE_COUNT) * 100.0f)) << "%" << endl;
		//}
	}
}

void Kernel::lockParticles() {
	int i = 0;
	int i_size = u_to_i(PARTICLE_COUNT);
	#pragma omp parallel for private(i) num_threads(12)
	for (i = 0; i < i_size; i++) {
		CPU_Particle& particle = particles[i];

		vector<CPU_Neighbor> neighbors;

		for (uint j = 0; j < PROBE_COUNT; j++) {
			if (i != j) {
				const dvec1 dist = glm::distance(particle.transformed_position, probes[j].transformed_position);
				neighbors.push_back(CPU_Neighbor(dist, &probes[j]));
			}
		}

		sort(neighbors.begin(), neighbors.end(), [](const CPU_Neighbor& a, const CPU_Neighbor& b) {
			return a.distance < b.distance;
		});

		particle.probe = neighbors[0].probe;

		//if ((i % (PROBE_COUNT / 5)) == 0) {
		//	#pragma omp critical
		//	cout << "Lock Particles: " << f_to_u(round(u_to_f(i) / u_to_f(PROBE_COUNT) * 100.0f)) << "%" << endl;
		//}
	}
}


void Kernel::simulate(const dvec1& delta_time) {
	DT = clamp(delta_time, 0.0, 0.25) * TIME_SCALE;
	SDT = DT / u_to_d(SUB_SAMPLES);

	const dvec1 day_time = DAY_TIME * 24.0;
	CALENDAR_HOUR = int(round(day_time - glm::fract(day_time)));
	CALENDAR_MINUTE = int(round(glm::fract(day_time) * 60.0));
	for (uint i = 0; i < SUB_SAMPLES; i++) {
		updateTime();
		sun_dir = sunDir();
		for (CPU_Probe& probe : probes) {
			probe.new_data = probe.data;
		}

		// SCATTER
		for (CPU_Probe& probe : probes) {
			updateProbePosition(&probe);
			calculateSunlight(&probe);
			scatterSPH(&probe);
		}

		// GATHER
		for (CPU_Probe& probe : probes) {
			gatherWind(&probe);
			gatherThermodynamics(&probe);

			probe.data = probe.new_data;
		}

	}

	auto first = &probes[0];
	compute_particles.clear();
	for (CPU_Particle& particle : particles) {
		updateParticlePosition(&particle);
		calculateParticle(&particle);
		updateParticlePosition(&particle);
		compute_particles.push_back(Compute_Particle(particle, first));
	}

	updateGPUParticles();
	updateGPUProbes();
}

void Kernel::updateTime() {
	const array<int, 12> daysInMonth = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	DAY_TIME += SDT * 0.01;
	if (DAY_TIME > 1.0) {
		DAY_TIME -= 1.0;
		if (DAY < 365)
			DAY++;
		else DAY = 0;
		YEAR_TIME = u_to_d(DAY) / 365.0;
		calculateDate();
	}
}

void Kernel::updateProbePosition(CPU_Probe* probe) const {
	const dvec1 axialTilt = -glm::radians(EARTH_TILT);
	const dvec1 theta = glm::radians(DAY_TIME * 360.0);

	const dquat tiltRotation = glm::angleAxis(axialTilt, dvec3(0, 0, 1));
	const dquat timeRotation = glm::angleAxis(theta, dvec3(0, 1, 0));
	const dquat combinedRotation = tiltRotation * timeRotation;

	probe->transformed_position = combinedRotation * probe->data.position;
}

void Kernel::scatterSPH(CPU_Probe* probe) const {
	probe->sph = CPU_Probe_Data();
	if (probe->neighbors.size() > 0) {
		for (CPU_Neighbor& neighbor : probe->neighbors) {
			const dvec1 smoothing_kernel = pow(glm::max(0.0, probe->smoothing_radius - neighbor.distance), 3.0);
			probe->sph.temperature += neighbor.probe->data.temperature;
		}
		probe->sph.temperature += probe->data.temperature;
		probe->sph.temperature /= ul_to_d(probe->neighbors.size() + 1);
		scatterWind(probe);
	}
	else {
		probe->sph = probe->data;
	}
}

void Kernel::scatterWind(CPU_Probe* probe) const {
	dvec3 wind = dvec3(0);
	dvec3 pressure_gradient = dvec3(0);

	for (CPU_Neighbor& neighbor : probe->neighbors) {
		const dvec1 smoothing_kernel = pow(glm::max(0.0, probe->smoothing_radius - neighbor.distance), 3.0);

		const dvec3 direction = neighbor.probe->data.position - probe->data.position;

		const dvec3 unitDirection = direction / neighbor.distance;
		const dvec1 pressureDifference = (probe->data.pressure - neighbor.probe->data.pressure) * 10.0;
		pressure_gradient += unitDirection * (pressureDifference) * smoothing_kernel;
		wind += neighbor.probe->data.wind_vector * smoothing_kernel;
	}

	wind /= ul_to_d(probe->neighbors.size());
	pressure_gradient /= ul_to_d(probe->neighbors.size());
	probe->sph.wind_vector = wind + pressure_gradient;
}

void Kernel::traceInitProperties(CPU_Probe* probe) const {
	const dvec3 ray_direction = glm::normalize(dvec3(0) - probe->transformed_position);

	const dvec1 a = glm::dot(ray_direction, ray_direction);
	const dvec1 b = 2.0 * dot(ray_direction, probe->transformed_position);
	const dvec1 c = dot(probe->transformed_position, probe->transformed_position) - 40.589641; // Earth Radius ^2
	const dvec1 delta = b * b - 4.0 * a * c;
	if (delta < 0.0) {
		return;
	}

	const dvec3 intersectionPoint = probe->transformed_position + ((-b - sqrt(delta)) / (2.0 * a)) * ray_direction;
	const dvec1 axialTilt = -glm::radians(EARTH_TILT);
	const dmat3 tiltRotation = dmat3(
		dvec3(cos(axialTilt), -sin(axialTilt), 0),
		dvec3(sin(axialTilt), cos(axialTilt), 0),
		dvec3(0, 0, 1));


	const dvec3 normal = tiltRotation * glm::normalize(intersectionPoint);

	const dvec1 theta = acos(normal.y);
	const dvec1 phi = glm::atan(normal.z, normal.x);

	const dvec2 uv = dvec2(glm::fract(1.0 - ((phi + PI) / TWO_PI) - DAY_TIME), (theta) / PI);

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
	const vec1 albedo_sample                        = textures.at(Texture_Field::ALBEDO                       ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 uv_index_sample                      = textures.at(Texture_Field::UV_INDEX                     ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 net_radiation_sample                 = textures.at(Texture_Field::NET_RADIATION                ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 solar_insolation_sample              = textures.at(Texture_Field::SOLAR_INSOLATION             ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 outgoiing_longwave_radiation_sample  = textures.at(Texture_Field::OUTGOING_LONGWAVE_RADIATION  ).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 reflected_shortwave_radiation_sample = textures.at(Texture_Field::REFLECTED_SHORTWAVE_RADIATION).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);

	const vec4 wind_vector_sample = textures.at(Texture_Field::WIND_VECTOR).sampleTexture(uv, Texture_Format::RGBA_8);

	{
		const vec1 topography = lut(Texture_Field::TOPOGRAPHY, topography_sample);
		const vec1 bathymetry = lut(Texture_Field::BATHYMETRY, bathymetry_sample);
		const vec1 sst = lut(Texture_Field::SEA_SURFACE_TEMPERATURE_DAY, sst_sample);
		const vec1 sst_night = lut(Texture_Field::SEA_SURFACE_TEMPERATURE_NIGHT, sst_night_sample);
		const vec1 lst = lut(Texture_Field::LAND_SURFACE_TEMPERATURE_DAY, lst_sample);
		const vec1 lst_night = lut(Texture_Field::LAND_SURFACE_TEMPERATURE_NIGHT, lst_night_sample);
		probe->data.pressure = lut(Texture_Field::SURFACE_PRESSURE, pressure_sample);

		if (topography == -1.0) { // Is at sea
			probe->data.day_temperature = sst;
			probe->data.night_temperature = sst_night;
			probe->data.on_water = true;
			probe->data.height = bathymetry;
			probe->data.albedo = 0.135;
		}
		else { // Is on Land
			probe->data.day_temperature = lst;
			probe->data.night_temperature = lst_night;
			probe->data.on_water = false;
			probe->data.height = topography;
			probe->data.albedo = lut(Texture_Field::ALBEDO, albedo_sample);
		}
		probe->data.temperature = glm::mix(probe->data.night_temperature, probe->data.day_temperature, probe->data.sun_intensity);

		probe->data.humidity = lut(Texture_Field::HUMIDITY, humidity_sample);
		probe->data.water_vapor = lut(Texture_Field::WATER_VAPOR, water_vapor_sample);
		probe->data.cloud_coverage = lut(Texture_Field::CLOUD_COVERAGE, cloud_coverage_sample);
		probe->data.cloud_water_content = lut(Texture_Field::CLOUD_WATER_CONTENT, cloud_water_content_sample);
		probe->data.cloud_particle_radius = lut(Texture_Field::CLOUD_PARTICLE_RADIUS, cloud_particle_radius_sample);
		probe->data.cloud_optical_thickness = lut(Texture_Field::CLOUD_OPTICAL_THICKNESS, cloud_optical_thickness_sample);

		probe->data.ozone = lut(Texture_Field::OZONE, ozone_sample);
		probe->data.uv_index = lut(Texture_Field::UV_INDEX, uv_index_sample);
		probe->data.net_radiation = lut(Texture_Field::NET_RADIATION, net_radiation_sample);
		probe->data.solar_insolation = lut(Texture_Field::SOLAR_INSOLATION, solar_insolation_sample);
		probe->data.outgoing_longwave_radiation = lut(Texture_Field::OUTGOING_LONGWAVE_RADIATION, outgoiing_longwave_radiation_sample);
		probe->data.reflected_shortwave_radiation = lut(Texture_Field::REFLECTED_SHORTWAVE_RADIATION, reflected_shortwave_radiation_sample);
		calculateSunlight(probe);
		probe->data.sun_intensity = probe->new_data.sun_intensity;
		probe->data.solar_irradiance = probe->new_data.solar_irradiance;
		probe->data.emissivity = clamp(probe->data.reflected_shortwave_radiation / (1360.0 - probe->data.solar_insolation), 0.0, 1.0);

		const dvec1 axialTilt = -glm::radians(EARTH_TILT);
		const dvec1 u = glm::radians(wind_vector_sample.x);
		const dvec1 v = glm::radians(wind_vector_sample.y);

		const dquat tiltRotation = glm::angleAxis(axialTilt, dvec3(0, 0, 1));
		const dquat uRotation  = glm::angleAxis(u,dvec3(0, 1, 0));
		const dquat vRotation  = glm::angleAxis(v, dvec3(1, 0, 0));

		probe->data.wind_quaternion = tiltRotation * uRotation * vRotation;
		probe->data.wind_u = wind_vector_sample.x;
		probe->data.wind_v = wind_vector_sample.y;
	}
}

void Kernel::calculateSunlight(CPU_Probe* probe) const {
	const dvec3 normal = glm::normalize(probe->transformed_position);
	probe->new_data.sun_intensity = clamp(dot(normal, sun_dir), 0.0, 1.0); // %
	probe->new_data.solar_irradiance = max((probe->data.sun_intensity * 1360.0) - probe->data.solar_insolation, 0.0); // W/m^2
}

void Kernel::gatherWind(CPU_Probe* probe) const {
	dvec3 wind = dvec3(0);
	for (CPU_Neighbor& neighbor : probe->neighbors) {
		const dvec1 inv_smoothing_kernel = pow(glm::max(0.0, neighbor.probe->smoothing_radius - neighbor.distance), 3.0);

		wind += neighbor.probe->sph.wind_vector * inv_smoothing_kernel * 1.5;
	}
	wind /= ul_to_d(probe->neighbors.size());
	probe->new_data.wind_vector *= (1.0 - SDT * 0.01);
	probe->new_data.wind_vector += (wind * SDT) * 2.5;
	//probe->new_data.wind_vector = glm::rotate(glm::angleAxis(glm::linearRand(-0.01, 0.01), glm::normalize(probe->transformed_position)), probe->new_data.wind_vector);
	// TODO implement coriolis
	const dvec1 wind_speed = glm::length(probe->new_data.wind_vector);
	if (wind_speed == 0.0) {
		probe->new_data.wind_vector = glm::normalize(dvec3(randD(1.0, 2.0), randD(1.0, 2.0), randD(1.0, 2.0)));
	}
	if (wind_speed < 0.005) {
		probe->new_data.wind_vector	= glm::normalize(probe->new_data.wind_vector) * 0.1;
		//cout << "Wind Too Slow: " << wind_speed << endl;
	}
	if (wind_speed > 40.0) {
		probe->new_data.wind_vector = glm::normalize(probe->new_data.wind_vector) * 35.0;
		//cout << "Wind Too Fast: " << wind_speed << endl;
	}
}

void Kernel::gatherThermodynamics(CPU_Probe* probe) const {
	// = solar_heat_transfer_coefficient * (absorptivity) * (solar irradiance) * area
	const dvec1 solar_heat_absorption = (1.0 - probe->data.albedo) * probe->data.solar_irradiance * probe->data.surface_area;

	// = emissivity * Stefan-Boltzmann * temperature * area
	const dvec1 radiative_loss = probe->data.emissivity * STEFAN_BOLZMANN * pow(probe->data.temperature, 4.0) * probe->data.surface_area;

	// = (convective_heat_transfer_coefficient) * (temperature - surrounding_temperature) * area
	const dvec1 wind_speed = glm::length(vec2(probe->data.wind_u, probe->data.wind_v));
	const dvec1 coeff = pow((1.0 + wind_speed), 0.4);
	const dvec1 convective_transfer = coeff * (probe->data.temperature - probe->sph.temperature) * probe->data.surface_area;

	const dvec1 net_heat = solar_heat_absorption * 0.001 - radiative_loss * 0.004 - convective_transfer * 0.001;
	if (abs(net_heat) > 1.0) {
		cout << "Temp Changing Too Quickly: Net  " << net_heat << "  | Solar  " << solar_heat_absorption << "  | Rad  -" << radiative_loss << "  | Convection  " << convective_transfer << endl;
	}
	probe->new_data.temperature += net_heat * SDT;
	probe->new_data.pressure += net_heat * SDT;
}

void Kernel::calculateParticle(CPU_Particle* particle) const {
	dvec1 distance = glm::distance(particle->transformed_position, particle->probe->transformed_position);

	// Recalculate Closest Probe TODO: Make Recursive
	bool foundClosest = false;
	while (!foundClosest) {
		foundClosest = true;
		for (const CPU_Neighbor& neighbor : particle->probe->neighbors) {
			const dvec1 dist = glm::distance(particle->transformed_position, neighbor.probe->transformed_position);
			if (dist < distance) {
				particle->probe = neighbor.probe;
				distance = dist;
				foundClosest = false;  // A closer probe was found, so keep searching
			}
		}
	}

	dquat wind_vector = dquat(1.0, 0.0, 0.0, 0.0);
	for (const CPU_Neighbor& neighbor : particle->probe->neighbors) {
		const dvec1 dist = glm::distance(particle->transformed_position, neighbor.probe->transformed_position);
		const dvec1 smoothing_kernel = pow(glm::max(0.0, 0.25 - dist), 3.0);
		wind_vector += neighbor.probe->data.wind_quaternion * smoothing_kernel;
	}

	// Add wind to velocity
	// Quaternion multiplication combines rotations
	particle->wind_speed = glm::normalize(glm::normalize((wind_vector * DT)) * particle->wind_speed);

	// Apply drag to velocity
	//dvec3 velVec(particle->wind_speed.x, particle->wind_speed.y, particle->wind_speed.z);
	//dvec3 dragForce = -DRAG_COEFFICIENT * velVec;
	//dquat dragQuat(0.0, dragForce.x, dragForce.y, dragForce.z);
	//particle->wind_speed += dragQuat * DT;
	//particle->wind_speed = glm::normalize(particle->wind_speed);

	dquat deltaQuat = glm::normalize(particle->wind_speed * particle->rotation);
	particle->rotation += deltaQuat * (0.01 * DT);
	particle->rotation = glm::normalize(particle->rotation);
}

dvec3 Kernel::rotateGeoloc(const dvec3& point, const dvec2& geoloc) const {
	const dvec1 phi = glm::radians(geoloc.x - 90.0);
	const dvec1 theta = glm::radians(geoloc.y + 90.0);

	const dquat latRotation  = glm::angleAxis(phi, dvec3(1, 0, 0));
	const dquat lonRotation  = glm::angleAxis(theta,dvec3(0, 1, 0));
	const dquat combinedRotation =  lonRotation * latRotation;

	return combinedRotation * point;
}

dquat Kernel::rotateGeoloc(const dvec2& geoloc) const {
	const dvec1 phi = glm::radians(geoloc.x - 90.0);
	const dvec1 theta = glm::radians(geoloc.y + 90.0);

	const dquat latRotation  = glm::angleAxis(phi, dvec3(1, 0, 0));
	const dquat lonRotation  = glm::angleAxis(theta,dvec3(0, 1, 0));
	const dquat combinedRotation = lonRotation * latRotation;

	return combinedRotation;
}

void Kernel::updateParticlePosition(CPU_Particle* particle) const {
	const dvec1 axialTilt = -glm::radians(EARTH_TILT);
	const dvec1 theta = glm::radians(DAY_TIME * 360.0);

	const dquat tiltRotation = glm::angleAxis(axialTilt, dvec3(0, 0, 1));
	const dquat timeRotation = glm::angleAxis(theta, dvec3(0, 1, 0));
	const dquat combinedRotation = tiltRotation * timeRotation * particle->rotation;

	particle->transformed_position = combinedRotation * particle->position;
}

dvec3 Kernel::sunDir() const {
	const dvec2 time = dvec2(0, YEAR_TIME * TWO_PI);
	const dvec2 c = cos(time);
	const dvec2 s = sin(time);

	const dmat3 rot = dmat3(
		c.y      ,  0.0, -s.y,
		s.y * s.x,  c.x,  c.y * s.x,
		s.y * c.x, -s.x,  c.y * c.x
	);
	return glm::normalize(dvec3(-1,0,0) * rot);
}

void Kernel::calculateDate() {
	const array<int, 12> daysInMonth = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int dayOfYear = d_to_i(round(YEAR_TIME * 365.0 + 355.0));
	if (dayOfYear > 365) {
		dayOfYear -= 365;
	}
	int daysPassed = 0;

	for (CALENDAR_MONTH = 0; CALENDAR_MONTH < 12; ++CALENDAR_MONTH) {
		if (dayOfYear <= daysPassed + daysInMonth[CALENDAR_MONTH]) {
			CALENDAR_DAY = dayOfYear - daysPassed;
			++CALENDAR_MONTH;
			return;
		}
		daysPassed += daysInMonth[CALENDAR_MONTH];
	}
}

void Kernel::calculateDateTime() {
	calculateYearTime();
	calculateDayTime();
}

void Kernel::calculateYearTime() {
	const array<int, 12> daysInMonth = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int dayOfYear = 0;
	for (int i = 0; i < CALENDAR_MONTH - 1; ++i) {
		dayOfYear += daysInMonth[i];
	}
	dayOfYear += CALENDAR_DAY;

	const int totalDaysInYear = 365;
	dvec1 adjustedValue = (355 - dayOfYear + 1) / i_to_d(totalDaysInYear); // Start in solstice december 21

	if (adjustedValue < 0) {
		adjustedValue = 1.0 + adjustedValue;
	}
	YEAR_TIME = adjustedValue;
}

void Kernel::calculateDayTime() {
	DAY_TIME = (i_to_d(CALENDAR_HOUR) + (i_to_d(CALENDAR_MINUTE) / 60.0)) / 24.0;
}