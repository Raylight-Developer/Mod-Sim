﻿#include "Kernel.hpp"

#define AIR_DENSITY        1.225      // kg/m^3 (at sea level)
#define GAS_CONSTANT       287.05     // J/(kg·K) for dry air
#define EARTH_ROTATION     7.2921e-5  // radians per second
#define SPECIFIC_HEAT_AIR  1005.0     // J/(kg·K)
#define STEFAN_BOLTZMANN   5.67e-8    // W/m²·K⁴
#define HEAT_TRANSFER_RATE 0.6        // W/m·K
#define HEAT_LOSS_RATE     0.2        // W/m·K
#define LATITUDE           15.0

#define PARTICLE_COLLISION_SAMPLES 5
#define PARTICLE_RESTITUTION       0.95
#define RESTITUTION                0.95
#define GRAVITY                    dvec3(0.0, -9.81, 0.0)

#define ATMOSPHERE_TEMP    5.5        // C
#define SEA_SURFACE_TEMP   28.5       // C
#define AMBIENT_TEMP       15.5       // C
#define CORIOLIS           dvec3(2 * 15.0, 0, 0)

CPU_Particle::CPU_Particle() {
	mass = 0;
	density = 0;
	humidity = 0;
	pressure = 0;
	temperature = 0;

	position = dvec3(0);
	velocity = dvec3(0);
	acceleration = dvec3(0);

	colliding = false;
	cell = nullptr;
	cell_id = uvec3(0);
}

GPU_Particle::GPU_Particle() {
	position = vec3(0);
	temperature = 0;
	velocity = vec4(0);
}

GPU_Particle::GPU_Particle(const CPU_Particle& particle) {
	position = d_to_f(particle.position);
	temperature = d_to_f(particle.temperature);
	velocity = vec4(d_to_f(particle.velocity), 1);
}

CPU_Cell::CPU_Cell() {
	density  = 0.0;
	humidity = 0.0;
	pressure = 0.0;
	temperature = AMBIENT_TEMP;

	particles = {};
	particle_count = 0;

	pmin = dvec3(0);
	pmax = dvec3(0);

	velocity_field = dvec3(0);
	acceleration_field = dvec3(0);
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

Flip::Flip() {
	PARTICLE_RADIUS = SESSION_GET("PARTICLE_RADIUS", dvec1);
	PARTICLE_COUNT = ul_to_u(SESSION_GET("PARTICLE_COUNT", uint64));
	GRID_CELLS     = uvec3(SESSION_GET("GRID_SIZE_X", uint64),SESSION_GET("GRID_SIZE_Y", uint64),SESSION_GET("GRID_SIZE_Z", uint64));
	GRID_COUNT     = GRID_CELLS.x * GRID_CELLS.y * GRID_CELLS.z;
	CELL_SIZE      = SESSION_GET("CELL_SIZE", dvec1);
	INV_CELL_SIZE  = 1.0 / CELL_SIZE;
	GRID_SIZE      = dvec3(GRID_CELLS) * CELL_SIZE;
	HALF_SIZE      = GRID_SIZE * 0.5;
	REST_DENSITY = 0.0;
}

void Flip::init() {
	particles = vector(SESSION_GET("PARTICLE_COUNT", uint64), CPU_Particle());
	grid = vector(SESSION_GET("GRID_SIZE_X", uint64), vector(SESSION_GET("GRID_SIZE_Y", uint64), vector(SESSION_GET("GRID_SIZE_Z", uint64), CPU_Cell())));

	initParticles();
	initGrid();
}

void Flip::initParticles() {
	const dvec1 radius = 0.45;

	for (uint64 i = 0; i < particles.size(); i++) {
		CPU_Particle& particle = particles[i];
		const dvec1 normalized_i = i / (double)(particles.size() - 1);
		const dvec1 theta = acos(1.0 - 2.0 * normalized_i);
		const dvec1 phi = i * (glm::pi<dvec1>() * (3.0 - sqrt(5.0)));

		const dvec1 x = radius * sin(theta) * cos(phi);
		const dvec1 y = radius * cos(theta);
		const dvec1 z = radius * sin(theta) * sin(phi);

		particle.position = dvec3(x, y, z);
		particle.velocity = dvec3(randD(-0.5, 0.5), 0, randD(-0.5, 0.5));

		particle.mass = randD() * 0.0025 + 0.0025;
		particle.humidity = randD() * 0.5 + 0.1;
		particle.pressure = randD() * 0.5 + 0.1;
		particle.temperature = AMBIENT_TEMP + randD(-1.0, 1.0);

		seaThermalTransfer(particle, 0.05);
		atmosphereThermalTransfer(particle, 0.05);
	}
}

void Flip::initGrid() {
	for (uint x = 0; x < GRID_CELLS.x; ++x) {
		for (uint y = 0; y < GRID_CELLS.y; ++y) {
			for (uint z = 0; z < GRID_CELLS.z; ++z) {
				CPU_Cell& cell = grid[x][y][z];
				cell.pressure = 0.5;
				cell.density = 0.5;
				cell.pmin = dvec3(x, y, z) * CELL_SIZE - (GRID_SIZE * 0.5) ;
				cell.pmax = cell.pmin + CELL_SIZE;
				cell.temperature = AMBIENT_TEMP;
				//if (y == 0) {
				//	cell.temperature = SEA_SURFACE_TEMP;
				//}
			}
		}
	}
}

void Flip::simulate(const dvec1& delta_time) {
	integrate(delta_time);
	scatter(delta_time);
}

void Flip::integrate(const dvec1& delta_time) {
	for (CPU_Particle& particle : particles) {
		particle.acceleration = particle.mass * GRAVITY * delta_time;
		particle.velocity += particle.acceleration * delta_time;
		particle.position += particle.velocity * delta_time;

		seaThermalTransfer(particle, delta_time);
		atmosphereThermalTransfer(particle, delta_time);

		thermodynamics(particle, delta_time);

		boundingCollisions(particle);
	}
}

void Flip::thermodynamics(CPU_Particle& particle, const dvec1& delta_time) {

}

void Flip::seaThermalTransfer(CPU_Particle& particle, const dvec1& delta_time) {
	if (particle.position.y <= -HALF_SIZE.y * 0.5) {
		const dvec1 t = f_map(-HALF_SIZE.y, -HALF_SIZE.y * 0.5, 1.0, 0.0, clamp(particle.position.y, -HALF_SIZE.y, -HALF_SIZE.y * 0.5));
		particle.temperature = glm::mix(AMBIENT_TEMP, SEA_SURFACE_TEMP + randD(-1.0, 1.0), t);
	}
}

void Flip::atmosphereThermalTransfer(CPU_Particle& particle, const dvec1& delta_time) {
	if (particle.position.y >= HALF_SIZE.y * 0.5) {
		const dvec1 t = f_map(HALF_SIZE.y, HALF_SIZE.y * 0.5, 1.0, 0.0, clamp(particle.position.y, HALF_SIZE.y * 0.5, HALF_SIZE.y));
		particle.temperature = glm::mix(AMBIENT_TEMP, ATMOSPHERE_TEMP + randD(-1.0, 1.0), t);
	}
}

void Flip::scatter(const dvec1& delta_time) {
	const dvec1 normalized_density = (dvec1)(PARTICLE_COUNT / GRID_COUNT);
	for (uint x = 0; x < GRID_CELLS.x; ++x) {
		for (uint y = 0; y < GRID_CELLS.y; ++y) {
			for (uint z = 0; z < GRID_CELLS.z; ++z) {
				CPU_Cell& cell = grid[x][y][z];
				cell.particle_count = 0;
				for (const auto& particle : particles) {
					if (insideAABB(particle.position, cell.pmin, cell.pmax)) {
						cell.particle_count++;
						//cell.temperature += particle.temperature * HEAT_TRANSFER_RATE * delta_time;
					}
				}
				//cell.temperature -= HEAT_LOSS_RATE * (cell.temperature - AMBIENT_TEMP) * delta_time;
				cell.density = (dvec1)(cell.particle_count / normalized_density);

				dvec3 pressure_gradient = dvec3(0);
				for (int dx = -1; dx <= 1; dx++) {
					for (int dy = -1; dy <= 1; dy++) {
						for (int dz = -1; dz <= 1; dz++) {
							if (dx == 0 && dy == 0 && dz == 0) {
								continue;
							}
							uint ni = x + dx;
							uint nj = y + dy;
							uint nk = z + dz;

							if (ni >= 0 && ni < GRID_CELLS.x &&
								nj >= 0 && nj < GRID_CELLS.y &&
								nk >= 0 && nk < GRID_CELLS.z) {

								CPU_Cell& neighbor_cell = grid[ni][nj][nk];

								if (dx != 0) {
									pressure_gradient.x += (neighbor_cell.pressure - cell.pressure) / CELL_SIZE;
								}
								if (dy != 0) {
									pressure_gradient.y += (neighbor_cell.pressure - cell.pressure) / CELL_SIZE;
								}
								if (dz != 0) {
									pressure_gradient.z += (neighbor_cell.pressure - cell.pressure) / CELL_SIZE;
								}
							}
						}
					}
				}
				pressure_gradient /= 26.0;
				cell.velocity_field -= pressure_gradient * (1.0 / cell.density);
			}
		}
	}
}

void Flip::gather(const dvec1& delta_time) {

}

void Flip::particleCollisions(const dvec1& delta_time) {
	for (uint i = 0; i < PARTICLE_COLLISION_SAMPLES; i++) {
		for (uint x = 0; x < GRID_CELLS.x; ++x) {
			for (uint y = 0; y < GRID_CELLS.y; ++y) {
				for (uint z = 0; z < GRID_CELLS.z; ++z) {
					CPU_Cell& cell = grid[x][y][z];
					cell.particles.clear();
				}
			}
		}
		for (CPU_Particle& particle : particles) {
			for (uint x = 0; x < GRID_CELLS.x; ++x) {
				for (uint y = 0; y < GRID_CELLS.y; ++y) {
					for (uint z = 0; z < GRID_CELLS.z; ++z) {
						CPU_Cell& cell = grid[x][y][z];
						if (insideAABB(particle.position, cell.pmin, cell.pmax)) {
							cell.particles.push_back(&particle);
							particle.cell = &cell;
							particle.cell_id = uvec3(x, y, z);
						}
					}
				}
			}
		}

		for (CPU_Particle& particle : particles) {
			// Neighboring Cells
			CPU_Cell* cell = particle.cell;
			const uvec3 cell_id = particle.cell_id;
			vector<CPU_Particle*> neighbors;
			for (int dx = -1; dx <= 1; dx++) {
				for (int dy = -1; dy <= 1; dy++) {
					for (int dz = -1; dz <= 1; dz++) {
						if (dx == 0 && dy == 0 && dz == 0) {
							continue;
						}
						const uint x = cell_id.x + dx;
						const uint y = cell_id.y + dy;
						const uint z = cell_id.z + dz;
						if (x >= 0 && x < GRID_CELLS.x && y >= 0 && y < GRID_CELLS.y && z >= 0 && z < GRID_CELLS.z) {
							neighbors.insert(neighbors.end(), grid[x][y][z].particles.begin(), grid[x][y][z].particles.end());
						}
					}
				}
			}
			for (CPU_Particle* neighbor : neighbors) {
				if (neighbor != &particle) {
					if (resolveOverlap(&particle, neighbor)) {
						resolveCollision(&particle, neighbor);
					}
				}
			}
			boundingCollisions(particle);
		}
	}
}

void Flip::particleCollisionsUnoptimized(const dvec1& delta_time) {
	for (uint i = 0; i < PARTICLE_COLLISION_SAMPLES; i++) {
		for (uint64 j = 0; j < particles.size(); j++) {
			CPU_Particle& particle = particles[j];
			for (uint64 k = 0; k < particles.size(); k++) {
				CPU_Particle& neighbor = particles[k];
				if (j != k) {
					if (resolveOverlap(&particle, &neighbor)) {
						resolveCollision(&particle, &neighbor);
;					}
				}
			}
		}
	}
	for (CPU_Particle& particle: particles) {
		boundingCollisions(particle);
	}
}

void Flip::boundingCollisions(CPU_Particle& particle) {
	particle.colliding = false;
	if (particle.position.x - PARTICLE_RADIUS < 0.0 - HALF_SIZE.x) {
		particle.position.x = (PARTICLE_RADIUS - HALF_SIZE.x);
		particle.velocity.x = (-particle.velocity.x * RESTITUTION);
		particle.colliding = true;
	}
	else if (particle.position.x + PARTICLE_RADIUS >  HALF_SIZE.x ) {
		particle.position.x = (HALF_SIZE.x - PARTICLE_RADIUS);
		particle.velocity.x = (-particle.velocity.x * RESTITUTION);
		particle.colliding = true;
	}

	if (particle.position.y - PARTICLE_RADIUS < 0.0 - HALF_SIZE.y) {
		particle.position.y = (PARTICLE_RADIUS - HALF_SIZE.y);
		particle.velocity.y = (-particle.velocity.y * RESTITUTION);
		particle.colliding = true;
	}
	else if (particle.position.y + PARTICLE_RADIUS >  HALF_SIZE.y ) {
		particle.position.y = (HALF_SIZE.y - PARTICLE_RADIUS);
		particle.velocity.y = (-particle.velocity.y * RESTITUTION);
		particle.colliding = true;
	}

	if (particle.position.z - PARTICLE_RADIUS < 0.0 - HALF_SIZE.z) {
		particle.position.z = (PARTICLE_RADIUS - HALF_SIZE.z);
		particle.velocity.z = (-particle.velocity.z * RESTITUTION);
		particle.colliding = true;
	}
	else if (particle.position.z + PARTICLE_RADIUS >  HALF_SIZE.z ) {
		particle.position.z = (HALF_SIZE.z - PARTICLE_RADIUS);
		particle.velocity.z = (-particle.velocity.z * RESTITUTION);
		particle.colliding = true;
	}
}

bool Flip::resolveOverlap(CPU_Particle* particle_a, CPU_Particle* particle_b) {
	const dvec3 distance_vector = particle_b->position - particle_a->position;
	const dvec1 distance = glm::length(distance_vector);
	const dvec1 overlap = (PARTICLE_RADIUS + PARTICLE_RADIUS) - distance;

	if (overlap > 0) {
		const dvec3 direction = distance_vector / distance;
		const dvec3 correction = direction * (overlap / 2.0);

		particle_a->position -= correction;
		particle_b->position += correction;
		return true;
	}
	return false;
}

void Flip::resolveCollision(CPU_Particle* particle_a, CPU_Particle* particle_b) {
	const dvec3 collision_normal = glm::normalize(particle_b->position - particle_a->position);
	const dvec3 relative_velocity = particle_b->velocity - particle_a->velocity;
	const dvec1 velocity_along_normal = glm::dot(relative_velocity, collision_normal);

	if (velocity_along_normal > 0.0) {
		return;
	}

	const dvec1 impulse_scalar = (-(1.0 + PARTICLE_RESTITUTION) * velocity_along_normal) /
		(1.0 / particle_a->mass + 1.0 / particle_b->mass);
	const dvec3 impulse = impulse_scalar * collision_normal;
	particle_a->velocity -= impulse / particle_a->mass;
	particle_b->velocity += impulse / particle_b->mass;
}