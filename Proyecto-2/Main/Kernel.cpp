#include "Kernel.hpp"

#include "Lut.hpp"

#define STEFAN_BOLZMANN 5.67e-8
#define AIR_SPECIFIC_HEAT_CAPACITY 1005.0 // J/kg°C
#define AIR_DENSITY                1.225 // kg/m³

#define CORIOLIS           vec3(15.0, 0, 0)

#pragma optimize("O3", on)
Kernel::Kernel() {
	PARTICLE_RADIUS    = 0.015f;
	PARTICLE_COUNT     = 4096;// 8192 * 2;
	BVH_SPH            = false;
	MAX_OCTREE_DEPTH   = 2;
	POLE_BIAS          = 0.975;
	POLE_BIAS_POWER    = 5.0;
	POLE_GEOLOCATION   = dvec2(25.0, 90.0);
	EARTH_TILT         = 23.5;
	CALENDAR_DAY       = 21;
	CALENDAR_MONTH     = 12;
	CALENDAR_HOUR      = 21;
	CALENDAR_MINUTE    = 0;
	YEAR_TIME          = 0;
	DAY_TIME           = 0;
	DAY                = 0;
	TIME_SCALE         = 1.0;
	DT                 = 0;
	RUNFRAME           = 0;
	SAMPLES            = 4;
	SDT                = 0;
	time               = 0;
	frame_count        = 0;
	sun_dir            = dvec3(0, 0, 1);
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

void Kernel::buildParticles() {
	cout << "Rebuild Particles" << endl;
	const dvec1 radius = 6.371 + PARTICLE_RADIUS;
	sun_dir = sunDir();

	particles.clear();
	for (uint i = 0; i < PARTICLE_COUNT; i++) {
		CPU_Particle particle = CPU_Particle();
		const dvec1 normalized_i = i / (dvec1)(PARTICLE_COUNT - 1);
		const dvec1 biased_i = (1.0 - POLE_BIAS) * normalized_i + POLE_BIAS * pow(normalized_i, POLE_BIAS_POWER);

		const dvec1 theta = acos(1.0 - 2.0 * biased_i);
		const dvec1 phi = dvec1(i) * (glm::pi<dvec1>() * (3.0 - sqrt(5.0)));

		const dvec1 x = radius * sin(theta) * cos(phi);
		const dvec1 y = radius * cos(theta);
		const dvec1 z = radius * sin(theta) * sin(phi);

		particle.data.position = rotateGeoloc(dvec3(x, y, z), POLE_GEOLOCATION);
		rotateEarth(&particle);

		traceInitProperties(&particle);
		particle.gen_index = i;
		particles.push_back(particle);
	}
}

void Kernel::lock() {
	textures.clear();
	lockParticles();
	time = 0.0;
	frame_count = 0;
}

void Kernel::lockParticles() {
	const int NUM_NEIGHBORS = 6;

	int i = 0;
	int i_size = u_to_i(PARTICLE_COUNT);
	#pragma omp parallel for private(i) num_threads(12)
	for (i = 0; i < i_size; i++) {
		CPU_Particle& particle = particles[i];

		vector<CPU_Neighbor> neighbors;

		for (uint j = 0; j < PARTICLE_COUNT; j++) {
			if (i != j) {
				const dvec1 dist = glm::distance(particle.data.position, particles[j].data.position);
				neighbors.push_back(CPU_Neighbor(dist, &particles[j]));
			}
		}

		sort(neighbors.begin(), neighbors.end(), [](const CPU_Neighbor& a, const CPU_Neighbor& b) {
			return a.distance < b.distance;
		});

		particle.smoothing_radius = neighbors[NUM_NEIGHBORS].distance;
		for (uint k = 0; k < NUM_NEIGHBORS; k++) {
			particle.neighbors.push_back(neighbors[k]);
		}

		#pragma omp critical
		if ((i % (PARTICLE_COUNT / 5)) == 0) {
			cout << "Lock Particles: " << f_to_u(round(u_to_f(i) / u_to_f(PARTICLE_COUNT) * 100.0f)) << "%" << endl;
		}
	}
}

void Kernel::generateSPHTexture() {
	sph_texture = Texture();
	sph_texture.resolution = uvec2(360, 180);
	sph_texture.uint_data = vector(360 * 180, MAX_UINT32);

	int x = 0;
	int y = 0;
	int x_size = u_to_i(sph_texture.resolution.x);
	int y_size = u_to_i(sph_texture.resolution.y);
	#pragma omp parallel for private(x,y) collapse(2) num_threads(12)
	for (x = 0; x < x_size; x++) {
		for (y = 0; y < y_size; y++) {
			const dvec1 u = i_to_d(x) / i_to_d(x_size);
			const dvec1 v = i_to_d(y) / i_to_d(y_size);

			const dvec1 radius = 6.371;
			const dvec1 theta = u * glm::two_pi<dvec1>();
			const dvec1 phi = v * glm::pi<dvec1>();

			const dvec1 xPos = radius * sin(phi) * cos(theta);
			const dvec1 yPos = radius * cos(phi);
			const dvec1 zPos = radius * sin(phi) * sin(theta);

			const dvec3 position(xPos, yPos, zPos);

			vector<CPU_Neighbor> neighbors;

			for (uint i = 0; i < PARTICLE_COUNT; i++) {
				const dvec1 dist = glm::distance(position, particles[i].data.position);
				neighbors.push_back(CPU_Neighbor(dist, &particles[i]));
			}
			sort(neighbors.begin(), neighbors.end(), [](const CPU_Neighbor& a, const CPU_Neighbor& b) {
				return a.distance < b.distance;
			});

			for (uint k = 0; k < 3; k++) {
				sph_texture.uint_data[x + x_size * y] = neighbors[k].neighbor->gen_index;
			}
		}
	}
}

void Kernel::buildBvh() {
	const Builder bvh_build = BVH_SPH ? Builder(particles, -1.0f, 1): Builder(particles, d_to_f(PARTICLE_RADIUS), MAX_OCTREE_DEPTH);
	particles = bvh_build.particles;
	bvh_nodes = bvh_build.nodes;

	gpu_particles.clear();
	for (const CPU_Particle& particle : particles) {
		gpu_particles.push_back(GPU_Particle(particle));
	}
}

void Kernel::simulate(const dvec1& delta_time) {
	DT = delta_time * TIME_SCALE;
	SDT = DT / u_to_d(SAMPLES);
	gpu_particles.clear();

	const dvec1 day_time = DAY_TIME * 24.0;
	CALENDAR_HOUR = int(round(day_time - glm::fract(day_time)));
	CALENDAR_MINUTE = int(round(glm::fract(day_time) * 60.0));
	for (uint i = 0; i < SAMPLES; i++) {
		time += SDT;
		updateTime();
		sun_dir = sunDir();
		for (CPU_Particle& particle : particles) {
			particle.new_data = particle.data;
		}

		for (CPU_Particle& particle : particles) {
			rotateEarth(&particle);
			calculateSunlight(&particle);
			scatterSPH(&particle);
		}

		for (CPU_Particle& particle : particles) {
			gatherPressure(&particle);
			gatherThermodynamics(&particle);

			particle.new_data.wind_vector = particle.new_data.pressure_gradient;



		}

//		if (i == SAMPLES - 1) {
//			generateSPHTexture();
//		}

		for (CPU_Particle& particle : particles) {
			particle.data = particle.new_data;
		}
		frame_count++;
	}

	for (const CPU_Particle& particle : particles) {
		gpu_particles.push_back(GPU_Particle(particle));
	}
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

void Kernel::rotateEarth(CPU_Particle* particle) const {
	const dvec1 axialTilt = -glm::radians(EARTH_TILT);
	const dvec1 theta = glm::radians(DAY_TIME * 360.0);

	const dquat tiltRotation = glm::angleAxis(axialTilt, dvec3(0, 0, 1));
	const dquat timeRotation = glm::angleAxis(theta, dvec3(0, 1, 0));
	const dquat combinedRotation = tiltRotation * timeRotation;

	particle->transformed_position = combinedRotation * particle->data.position;
}

void Kernel::scatterSPH(CPU_Particle* particle) const {
	particle->sph = CPU_Particle_Data();
	if (particle->neighbors.size() > 0) {
		for (CPU_Neighbor& neighbor : particle->neighbors) {
			const dvec1 smoothing_kernel = pow(glm::max(0.0, particle->smoothing_radius - neighbor.distance), 3.0);
			particle->sph.temperature += neighbor.neighbor->data.temperature;
			particle->sph.wind_vector += neighbor.neighbor->data.wind_vector;

		}
		particle->sph.temperature += particle->data.temperature;
		particle->sph.wind_vector += particle->data.wind_vector;
		particle->sph.temperature /= ul_to_d(particle->neighbors.size() + 1);
		particle->sph.wind_vector /= ul_to_d(particle->neighbors.size() + 1);
		scatterPressure(particle);
	}
	else {
		particle->sph = particle->data;
	}
}

void Kernel::scatterPressure(CPU_Particle* particle) const {
	for (CPU_Neighbor& neighbor : particle->neighbors) {
		const dvec1 smoothing_kernel = pow(glm::max(0.0, particle->smoothing_radius - neighbor.distance), 3.0);

		const dvec3 direction = neighbor.neighbor->data.position - particle->data.position;
		const dvec1 distance = glm::length(direction);

		const dvec3 unitDirection = direction / distance;
		const dvec1 pressureDifference = particle->data.pressure - neighbor.neighbor->data.pressure;  // Pressure difference from the reference
		particle->sph.pressure_gradient += unitDirection * (pressureDifference) * smoothing_kernel; // Finite difference contribution
	}

	particle->sph.pressure_gradient /= ul_to_d(particle->neighbors.size());
}

void Kernel::traceInitProperties(CPU_Particle* particle) const {
	const dvec3 ray_direction = glm::normalize(dvec3(0) - particle->transformed_position);

	const dvec1 a = glm::dot(ray_direction, ray_direction);
	const dvec1 b = 2.0 * dot(ray_direction, particle->transformed_position);
	const dvec1 c = dot(particle->transformed_position, particle->transformed_position) - 40.589641; // Earth Radius ^2
	const dvec1 delta = b * b - 4.0 * a * c;
	if (delta < 0.0) {
		return;
	}

	const dvec3 intersectionPoint = particle->transformed_position + ((-b - sqrt(delta)) / (2.0 * a)) * ray_direction;
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

	const vec1 topography   = lut(Texture_Field::TOPOGRAPHY, topography_sample);
	const vec1 bathymetry   = lut(Texture_Field::BATHYMETRY, bathymetry_sample);
	const vec1 sst          = lut(Texture_Field::SEA_SURFACE_TEMPERATURE_DAY, sst_sample);
	const vec1 sst_night    = lut(Texture_Field::SEA_SURFACE_TEMPERATURE_NIGHT, sst_night_sample);
	const vec1 lst          = lut(Texture_Field::LAND_SURFACE_TEMPERATURE_DAY, lst_sample);
	const vec1 lst_night    = lut(Texture_Field::LAND_SURFACE_TEMPERATURE_NIGHT, lst_night_sample);
	particle->data.pressure = lut(Texture_Field::SURFACE_PRESSURE, pressure_sample);
	if (topography == -1.0) { // Is at sea
		particle->data.day_temperature = sst;
		particle->data.night_temperature = sst_night;
		particle->data.on_water = true;
		particle->data.height = bathymetry;
		particle->data.albedo = 0.135;
	}
	else { // Is on Land
		particle->data.day_temperature = lst;
		particle->data.night_temperature = lst_night;
		particle->data.on_water = false;
		particle->data.height = topography;
		particle->data.albedo = lut(Texture_Field::ALBEDO, albedo_sample);
	}

	particle->data.humidity                = lut(Texture_Field::HUMIDITY, humidity_sample);
	particle->data.water_vapor             = lut(Texture_Field::WATER_VAPOR, water_vapor_sample);
	particle->data.cloud_coverage          = lut(Texture_Field::CLOUD_COVERAGE, cloud_coverage_sample);
	particle->data.cloud_water_content     = lut(Texture_Field::CLOUD_WATER_CONTENT, cloud_water_content_sample);
	particle->data.cloud_particle_radius   = lut(Texture_Field::CLOUD_PARTICLE_RADIUS, cloud_particle_radius_sample);
	particle->data.cloud_optical_thickness = lut(Texture_Field::CLOUD_OPTICAL_THICKNESS, cloud_optical_thickness_sample);

	particle->data.ozone                         = lut(Texture_Field::OZONE, ozone_sample);
	particle->data.uv_index                      = lut(Texture_Field::UV_INDEX, uv_index_sample);
	particle->data.net_radiation                 = lut(Texture_Field::NET_RADIATION, net_radiation_sample);
	particle->data.solar_insolation              = lut(Texture_Field::SOLAR_INSOLATION, solar_insolation_sample);
	particle->data.outgoing_longwave_radiation   = lut(Texture_Field::OUTGOING_LONGWAVE_RADIATION, outgoiing_longwave_radiation_sample);
	particle->data.reflected_shortwave_radiation = lut(Texture_Field::REFLECTED_SHORTWAVE_RADIATION, reflected_shortwave_radiation_sample);
	calculateSunlight(particle);
	particle->data.sun_intensity = particle->new_data.sun_intensity;
	particle->data.solar_irradiance = particle->new_data.solar_irradiance;
	particle->data.temperature = glm::mix(particle->data.night_temperature, particle->data.day_temperature, particle->data.sun_intensity);
	particle->data.emissivity = clamp(particle->data.reflected_shortwave_radiation / (1360.0 - particle->data.solar_insolation), 0.0, 1.0);
}

void Kernel::calculateSunlight(CPU_Particle* particle) const {
	const dvec3 normal = glm::normalize(particle->transformed_position);
	particle->new_data.sun_intensity = clamp(dot(normal, sun_dir), 0.0, 1.0); // %
	particle->new_data.solar_irradiance = max((particle->data.sun_intensity * 1360.0) - particle->data.solar_insolation, 0.0); // W/m^2
}

void Kernel::gatherPressure(CPU_Particle* particle) const {
	for (CPU_Neighbor& neighbor : particle->neighbors) {
		const dvec1 inv_smoothing_kernel = pow(glm::max(0.0, neighbor.neighbor->smoothing_radius - neighbor.distance), 3.0);

		particle->new_data.pressure_gradient += neighbor.neighbor->sph.pressure_gradient;
		particle->new_data.pressure += neighbor.neighbor->data.pressure;
	}
	particle->new_data.pressure_gradient += particle->data.pressure_gradient;
	particle->new_data.pressure_gradient /= ul_to_d(particle->neighbors.size() + 1);
	particle->new_data.pressure += particle->data.pressure;
	particle->new_data.pressure /= ul_to_d(particle->neighbors.size() + 1);
}

void Kernel::gatherThermodynamics(CPU_Particle* particle) const {
	// = solar_heat_transfer_coefficient * (absorptivity) * (solar irradiance) * area
	const dvec1 solar_heat_absorption = (1.0 - particle->data.albedo) * particle->data.solar_irradiance * particle->data.surface_area;

	// = emissivity * Stefan-Boltzmann * temperature * area
	const dvec1 radiative_loss = particle->data.emissivity * STEFAN_BOLZMANN * pow(particle->data.temperature, 4.0) * particle->data.surface_area;

	// = (convective_heat_transfer_coefficient) * (temperature - surrounding_temperature) * area
	const dvec1 wind_speed = glm::length(particle->data.wind_vector);
	const dvec1 coeff = (5.0 + wind_speed);
	const dvec1 convective_transfer = coeff * (particle->data.temperature - particle->sph.temperature) * particle->data.surface_area;

	const dvec1 net_heat = solar_heat_absorption * 0.001 - radiative_loss * 0.005 - convective_transfer * 0.001;
	if (abs(net_heat) > 1.0) {
		cout << "Temp Changing Too Quickly: Net  " << net_heat << "  | Solar  " << solar_heat_absorption << "  | Rad  -" << radiative_loss << "  | Convection  " << convective_transfer << endl;
	}
	particle->new_data.temperature += net_heat * SDT;
}

dvec3 Kernel::rotateGeoloc(const dvec3& point, const dvec2& geoloc) const {
	const dvec1 phi = glm::radians(geoloc.x - 90.0);
	const dvec1 theta = glm::radians(geoloc.y + 90.0);

	const dquat latRotation  = glm::angleAxis(phi, dvec3(1, 0, 0));
	const dquat lonRotation  = glm::angleAxis(theta,dvec3(0, 1, 0));
	const dquat combinedRotation =  lonRotation * latRotation;

	return combinedRotation * point;
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