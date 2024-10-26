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
	compute_layout     = uvec2(0);
	compute_resolution = uvec2(0);

	textures[Texture_Field::TOPOGRAPHY] = Texture::fromFile("./Resources/Nasa Earth Data/Topography.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::SST] = Texture::fromFile("./Resources/Nasa Earth Data/Sea Surface Temperature CAF.png", Texture_Format::MONO_FLOAT);
	textures[Texture_Field::LST] = Texture::fromFile("./Resources/Nasa Earth Data/Land Surface Temperature CAF.png", Texture_Format::MONO_FLOAT);
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
	//initParticles();
	time = 0.0f;
	frame_count = 0;
	compute_resolution = uvec2(1920, 1080);

	compute_layout.x = d_to_u(ceil(u_to_d(compute_resolution.x) / 32.0));
	compute_layout.y = d_to_u(ceil(u_to_d(compute_resolution.y) / 32.0));

	glDeleteTextures(1, &gl_data["buf B"]);
	glDeleteTextures(1, &gl_data["buf C"]);
	gl_data["buf B"] = renderLayer(compute_resolution);
	gl_data["buf C"] = renderLayer(compute_resolution);

	{
		auto confirmation = computeShaderProgram("Simulation/Pressure");
		if (confirmation) {
			glDeleteProgram(gl_data["prog B"]);
			gl_data["prog B"] =  confirmation.data;
		}
	}
	{
		auto confirmation = computeShaderProgram("Simulation/Wind");
		if (confirmation) {
			glDeleteProgram(gl_data["prog C"]);
			gl_data["prog C"] =  confirmation.data;
		}
	}

	vector<uint> texture_data;
	vector<GPU_Texture> textures;
	vector<string> texture_names = { "Topography" };
	for (const string& tex : texture_names) {
		Texture texture = Texture::fromFile("./Resources/Nasa Earth Data/" + tex + ".png", Texture_Format::RGBA_8);
		textures.push_back(GPU_Texture(ul_to_u(texture_data.size()), texture.resolution.x, texture.resolution.y, 0));
		auto data = texture.toRgba8Texture();
		texture_data.insert(texture_data.end(), data.begin(), data.end());
	}
	gl_data["ssbo 2"] = ssboBinding(ul_to_u(textures.size() * sizeof(GPU_Texture)), textures.data());
	gl_data["ssbo 3"] = ssboBinding(ul_to_u(texture_data.size() * sizeof(uint)), texture_data.data());
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

	const GLuint prog_B = gl_data["prog B"];
	const GLuint prog_C = gl_data["prog C"];

	glUseProgram(prog_B);
	glUniform1f (glGetUniformLocation(prog_B, "iTime"), time);
	glUniform1ui(glGetUniformLocation(prog_B, "iFrame"), frame_count);
	glUniform2ui(glGetUniformLocation(prog_B, "iResolution"), compute_resolution.x, compute_resolution.y);

	glBindImageTexture(0, gl_data["buf B"], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gl_data["ssbo 2"]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, gl_data["ssbo 3"]);

	glDispatchCompute(compute_layout.x, compute_layout.y, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glUseProgram(prog_C);
	glUniform1f (glGetUniformLocation(prog_C, "iTime"), time);
	glUniform1ui(glGetUniformLocation(prog_C, "iFrame"), frame_count);
	glUniform2ui(glGetUniformLocation(prog_C, "iResolution"), compute_resolution.x, compute_resolution.y);

	glBindImageTexture(0, gl_data["buf B"], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
	glBindImageTexture(1, gl_data["buf C"], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

	glDispatchCompute(compute_layout.x, compute_layout.y, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glUseProgram(0);
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

	const vec1 topography_sample = textures.at(Texture_Field::TOPOGRAPHY).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 sst_sample = textures.at(Texture_Field::SST).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);
	const vec1 lst_sample = textures.at(Texture_Field::LST).sampleTextureMono(uv, Texture_Format::MONO_FLOAT);

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