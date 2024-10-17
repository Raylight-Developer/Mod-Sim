﻿#include "Kernel.hpp"

#define AIR_DENSITY       1.225      // kg/m^3 (at sea level)
#define GAS_CONSTANT      287.05     // J/(kg·K) for dry air
#define EARTH_ROTATION    7.2921e-5  // radians per second
#define SPECIFIC_HEAT_AIR 1005.0     // J/(kg·K)
#define STEFAN_BOLTZMANN  5.67e-8    // W/m²·K⁴
#define SEA_SURFACE_TEMP  28.5       // C
#define LATITUDE          15.0
#define RESTITUTION       1.0
#define GRAVITY           dvec3(0.0, -9.81, 0.0)


CPU_Particle::CPU_Particle() {
	mass = randD() * 0.5 + 0.1;
	humidity = randD() * 0.5 + 0.1;
	pressure = randD() * 0.5 + 0.1;
	temperature = 15.0 + randD() * 10.0;

	position = dvec3(0.0);
	velocity = dvec3(0.0);
	acceleration = dvec3(0.0);

	colliding = false;
}

GPU_Particle::GPU_Particle() {
	position = vec4(.0f);
	velocity = vec4(0.0f);
}

GPU_Particle::GPU_Particle(const CPU_Particle& particle) {
	position = vec4(d_to_f(particle.position), 1.0f);
	velocity = vec4(d_to_f(particle.velocity), 1.0f);
}

GPU_Cell::GPU_Cell() {
	density = 0.0f;
	humidity = 0.0f;
	pressure = 0.0f;
	temperature = 0.0f;
}

GPU_Cell::GPU_Cell(const CPU_Cell& cell) {
	density     = d_to_f(cell.density);
	humidity    = d_to_f(cell.humidity);
	pressure    = d_to_f(cell.pressure);
	temperature = d_to_f(cell.temperature);
}

void initialize(vector<CPU_Particle>& points) {
	dvec1 radius = 0.45;

	for (uint64 i = 0; i < points.size(); i++) {
		CPU_Particle& particle = points[i];
		const dvec1 angle = i * (glm::pi<dvec1>() * (3.0 - sqrt(5.0)));
		const dvec1 r = radius * sqrt(i / (dvec1)(points.size() - 1));

		const dvec1 x = r * cos(angle);
		const dvec1 y = r * sin(angle);

		particle.mass = randD() * 0.5 + 0.1;
		particle.humidity = randD() * 0.5 + 0.1;
		particle.pressure = randD() * 0.5 + 0.1;
		particle.temperature = 15.0 + randD() * 10.0;

		particle.position = dvec3(x, 0, y);
		particle.velocity = dvec3(0.0);
		particle.acceleration = dvec3(0.0);
	}
}

void simulate(vector<CPU_Particle>& points, const dvec1& delta_time) {
	for (CPU_Particle& particle: points) {
		computeThermodynamics(particle);
		updateVelocity(particle, points, delta_time);
		updatePosition(particle, delta_time);
	}
}

void updateVelocity(CPU_Particle& particle, const vector<CPU_Particle>& neighbors, const dvec1& delta_time) {
	dvec3 navier_stokes_force = computeNavierStokes(particle, neighbors);
	dvec3 coriolis_force = computeCoriolisEffect(particle);

	//particle.acceleration = (navier_stokes_force + coriolis_force) / particle.mass;
	particle.acceleration = GRAVITY / particle.mass;
	particle.velocity += particle.acceleration * delta_time;
}

void updatePosition(CPU_Particle& particle, const dvec1& delta_time) {
	particle.position += particle.velocity * delta_time;
	handleBorderCollision(particle);
}

void handleBorderCollision(CPU_Particle& particle) {
	const dvec1 radius = SESSION_GET("PARTICLE_RADIUS", dvec1);
	const ulvec3 size = ulvec3(SESSION_GET("GRID_SIZE_X", uint64),SESSION_GET("GRID_SIZE_Y", uint64),SESSION_GET("GRID_SIZE_Z", uint64));
	const dvec3  half_size = ul_to_d(size) * SESSION_GET("CELL_SIZE", dvec1) * 0.5;

	particle.colliding = false;
	if (particle.position.x - radius < 0.0 - half_size.x) {
		particle.position.x = (radius - half_size.x);
		particle.velocity.x = (-particle.velocity.x * RESTITUTION);
		particle.colliding = true;
	}
	else if (particle.position.x + radius >  half_size.x ) {
		particle.position.x = (half_size.x - radius);
		particle.velocity.x = (-particle.velocity.x * RESTITUTION);
		particle.colliding = true;
	}

	if (particle.position.y - radius < 0.0 - half_size.y) {
		particle.position.y = (radius - half_size.y);
		particle.velocity.y = (-particle.velocity.y * RESTITUTION);
		particle.colliding = true;
	}
	else if (particle.position.y + radius >  half_size.y ) {
		particle.position.y = (half_size.y - radius);
		particle.velocity.y = (-particle.velocity.y * RESTITUTION);
		particle.colliding = true;
	}

	if (particle.position.z - radius < 0.0 - half_size.z) {
		particle.position.z = (radius - half_size.z);
		particle.velocity.z = (-particle.velocity.z * RESTITUTION);
		particle.colliding = true;
	}
	else if (particle.position.z + radius >  half_size.z ) {
		particle.position.z = (half_size.z - radius);
		particle.velocity.z = (-particle.velocity.z * RESTITUTION);
		particle.colliding = true;
	}
}

dvec3 computeNavierStokes(const CPU_Particle& particle, const vector<CPU_Particle>& neighbors) {
	dvec3 pressure_gradient(0.0);
	dvec3 velocity_diffusion(0.0);

	for (const auto& neighbor : neighbors) {
		dvec3 distance = particle.position - neighbor.position;
		dvec1 dist_squared = length2(distance);

		if (dist_squared > 0.0) {
			pressure_gradient += (neighbor.pressure - particle.pressure) / dist_squared * normalize(distance);
			velocity_diffusion += (neighbor.velocity - particle.velocity) / dist_squared;
		}
	}

	return -pressure_gradient + velocity_diffusion;
}

dvec3 computeCoriolisEffect(const CPU_Particle& particle) {
	const dvec1 coriolisParameter = 2.0 * EARTH_ROTATION * sin(LATITUDE);
	return coriolisParameter * cross(dvec3(0.0, 1.0, 0.0), particle.velocity);
}

dvec1 computeThermodynamics(CPU_Particle& particle) {
	dvec1 radiation_loss = STEFAN_BOLTZMANN * pow(particle.temperature, 4);
	dvec1 heat_gain = SEA_SURFACE_TEMP * particle.humidity * SPECIFIC_HEAT_AIR * (SEA_SURFACE_TEMP - particle.temperature);

	particle.temperature += (heat_gain - radiation_loss) / (particle.mass * SPECIFIC_HEAT_AIR);
	return particle.temperature;
}







void initialize(Grid& grid) {
	const ulvec3 size = ulvec3(SESSION_GET("GRID_SIZE_X", uint64),SESSION_GET("GRID_SIZE_Y", uint64),SESSION_GET("GRID_SIZE_Z", uint64));
	dvec3 center = dvec3(size) / 2.0;
	dvec1 max_distance = glm::max(glm::max(size.x, size.y), size.z) / 2.0;

	for (uint64 x = 0; x < size.x; ++x) {
		for (uint64 y = 0; y < size.y; ++y) {
			for (uint64 z = 0; z < size.z; ++z) {
				CPU_Cell& cell = grid[x][y][z];

				dvec3 current_pos = dvec3(x, y, z);
				dvec1 distance = glm::length(current_pos - center);
				if (distance <= max_distance) {
					cell.density = 1.0f - (distance / max_distance);
				} else {
					cell.density = 0.0f;
				}
				cell.density *= randD() * 0.5 + 0.5;
				cell.pressure = randD() * 0.5 + 0.5;
			}
		}
	}
}

void simulate(Grid& grid, const dvec1& delta_time) {
	const ulvec3 size = ulvec3(SESSION_GET("GRID_SIZE_X", uint64),SESSION_GET("GRID_SIZE_Y", uint64),SESSION_GET("GRID_SIZE_Z", uint64));
	for (uint64 x = 0; x < size.x; ++x) {
		for (uint64 y = 0; y < size.y; ++y) {
			for (uint64 z = 0; z < size.z; ++z) {
				CPU_Cell& cell = grid[x][y][z];
			}
		}
	}
}

void forceSolve(Grid& grid, const dvec1& delta_time) {
	const ulvec3 size = ulvec3(SESSION_GET("GRID_SIZE_X", uint64),SESSION_GET("GRID_SIZE_Y", uint64),SESSION_GET("GRID_SIZE_Z", uint64));
	for (uint64 x = 0; x < size.x; ++x) {
		for (uint64 y = 0; y < size.y; ++y) {
			for (uint64 z = 0; z < size.z; ++z) {
				ulvec3 position = ulvec3(x, y, z);
				CPU_Cell& cell = grid[x][y][z];
			}
		}
	}
}

dvec3 computePressureGradient(const Grid& grid, const uint64& x, const uint64& y, const uint64& z, const uvec3& size) {
	dvec3 gradient(0.0);

	// Check boundaries and calculate differences between neighboring cells
	if (x > 0) gradient.x += grid[x - 1][y][z].pressure - grid[x][y][z].pressure;
	if (x < size.x - 1) gradient.x += grid[x + 1][y][z].pressure - grid[x][y][z].pressure;

	if (y > 0) gradient.y += grid[x][y - 1][z].pressure - grid[x][y][z].pressure;
	if (y < size.y - 1) gradient.y += grid[x][y + 1][z].pressure - grid[x][y][z].pressure;

	if (z > 0) gradient.z += grid[x][y][z - 1].pressure - grid[x][y][z].pressure;
	if (z < size.z - 1) gradient.z += grid[x][y][z + 1].pressure - grid[x][y][z].pressure;

	return gradient;
}