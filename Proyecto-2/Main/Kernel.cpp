#include "Kernel.hpp"

#define AIR_GAS_CONSTANT   287.05f // J/(kg·K)
#define AIR_DENSITY        1.225f  // kg/m^3 (at sea level)

#define PARTICLE_RESTITUTION       0.95f
#define RESTITUTION                0.25f
#define GRAVITY                    vec3(0.0)

#define ATMOSPHERE_TEMP    5.5f        // C
#define SEA_SURFACE_TEMP   28.5f       // C
#define AMBIENT_TEMP       15.5f       // C

#define AIR_SPECIFIC_HEAT_CAPACITY 1005.0f // J/kg°C
#define AIR_DENSITY                1.225f // kg/m³

#define CORIOLIS           vec3(15.0f, 0, 0)

#define CELL_HEAT_GAIN             1.0f
#define CELL_AMBIENT_HEAT_TRANSFER 0.15f
#define HEAT_TRANSFER_COEFFICIENT  0.05f

CPU_Particle::CPU_Particle() {
	mass = 0;
	density = 0;
	temperature = 0;

	position = vec3(0);
	velocity = vec3(0);
	acceleration = vec3(0);

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
	position = particle.position;
	temperature = particle.temperature;
	velocity = vec4(particle.velocity, 1);
}

CPU_Cell::CPU_Cell() {
	density  = 0;
	humidity = 0;
	pressure = 0;
	temperature = AMBIENT_TEMP;

	particles = {};
	particle_count = 0;

	pmin = vec3(0);
	pmax = vec3(0);

	velocity_field = vec3(0);
	type = Cell_Type::AIR;
}

GPU_Cell::GPU_Cell() {
	density = 0;
	humidity = 0;
	pressure = 0;
	temperature = 0;

	pmin = vec3(0);
	a = 0;
	pmax = vec3(0);
	b = 0;
}

GPU_Cell::GPU_Cell(const CPU_Cell& cell) {
	density     = cell.density;
	humidity    = cell.humidity;
	pressure    = cell.pressure;
	temperature = cell.temperature;

	pmin = cell.pmin;
	pmax = cell.pmax;
	a = 0;
	b = 0;
}

Flip::Flip() {
}

void Flip::init(const vec1& PARTICLE_RADIUS, const uint& PARTICLE_COUNT, const uvec3& GRID_CELLS) {
	this->PARTICLE_RADIUS = PARTICLE_RADIUS;
	this->PARTICLE_COUNT  = PARTICLE_COUNT;
	this->GRID_CELLS      = GRID_CELLS;
	GRID_COUNT            = GRID_CELLS.x * GRID_CELLS.y * GRID_CELLS.z;
	CELL_SIZE             = 1.0f / u_to_f(max(max(GRID_CELLS.x, GRID_CELLS.y), GRID_CELLS.z));
	INV_CELL_SIZE         = 1.0f / CELL_SIZE;
	GRID_SIZE             = vec3(GRID_CELLS) * CELL_SIZE;
	HALF_SIZE             = GRID_SIZE * 0.5f;
	REST_DENSITY          = 0.0f;
	SMOOTH_RADIUS         = CELL_SIZE * 1.5;
	DT                    = 0.016f;
	RUNFRAME              = 0;
	SAMPLES               = 5;
	SDT                   = 0.016f / u_to_f(SAMPLES);

	particles.clear();
	grid.clear();
	for (uint i = 0; i < PARTICLE_COUNT; ++i) {
		particles.push_back(CPU_Particle());
	}
	for (uint i = 0; i < GRID_COUNT; ++i) {
		grid.push_back(CPU_Cell());
	}

	initParticles();
	initGrid();
	scatter();
}

void Flip::initParticles() {
	const vec1 radius = (0.5f - PARTICLE_RADIUS * 2.0f) *  min(min(GRID_SIZE.x, GRID_SIZE.y), GRID_SIZE.z);

	for (uint64 i = 0; i < particles.size(); i++) {
		CPU_Particle& particle = particles[i];
		const vec1 normalized_i = i / (vec1)(particles.size() - 1);
		const vec1 theta = acos(1.0f - 2.0f * normalized_i);
		const vec1 phi = vec1(i) * (glm::pi<vec1>() * (3.0f - sqrt(5.0f)));

		const vec1 x = radius * sin(theta) * cos(phi);
		const vec1 y = radius * cos(theta);
		const vec1 z = radius * sin(theta) * sin(phi);

		particle.position = vec3(x, y, z);
		particle.velocity = vec3(0);

		particle.mass = randF(0.5, 1.0);
		particle.temperature = AMBIENT_TEMP + randF(-1.0, 1.0);

		seaThermalTransfer(&particle);
		atmosphereThermalTransfer(&particle);
	}
}

void Flip::initGrid() {
	for (uint x = 0; x < GRID_CELLS.x; ++x) {
		for (uint y = 0; y < GRID_CELLS.y; ++y) {
			for (uint z = 0; z < GRID_CELLS.z; ++z) {
				CPU_Cell* cell = getGrid(x, y, z);
				cell->pressure = 0.5;
				cell->density = 0.5;
				cell->pmin = vec3(x, y, z) * CELL_SIZE - (GRID_SIZE * 0.5f) ;
				cell->pmax = cell->pmin + CELL_SIZE;
				cell->center = cell->pmin + CELL_SIZE / 2.0f;
				cell->temperature = AMBIENT_TEMP;
				uint i = 0;
				for (int dx = -1; dx <= 1; dx++) {
					for (int dy = -1; dy <= 1; dy++) {
						for (int dz = -1; dz <= 1; dz++) {
							if (dx == 0 && dy == 0 && dz == 0) {
								continue;
							}
							const uint ddx = x + dx;
							const uint ddy = y + dy;
							const uint ddz = z + dz;
							if (ddx >= 0 && ddx < GRID_CELLS.x && ddy >= 0 && ddy < GRID_CELLS.y && ddz >= 0 && ddz < GRID_CELLS.z) {
								cell->neighbors[i] = getGrid(ddx, ddy, ddz);
							}
							else {
								cell->neighbors[i] = nullptr;
							}
							i++;
						}
					}
				}
			}
		}
	}
	for (uint x = 0; x < GRID_CELLS.x; ++x) {
		for (uint y = 0; y < GRID_CELLS.y; ++y) {
			for (uint z = 0; z < GRID_CELLS.z; ++z) {
				CPU_Cell* cell = getGrid(x, y, z);
			}
		}
	}
}

void Flip::simulate(const dvec1& delta_time) {
	DT = d_to_f(delta_time);
	integrate();
	scatter();
	computeGrid();
	gather();
	computeParticles();
}

void Flip::integrate() {
	for (CPU_Particle& particle : particles) {
		particle.acceleration = particle.mass * GRAVITY * DT;
		particle.velocity += particle.acceleration * DT;
		particle.position += particle.velocity * DT;

		seaThermalTransfer(&particle);
		atmosphereThermalTransfer(&particle);

		boundingCollisions(&particle);
	}
}

void Flip::thermodynamics(CPU_Particle* particle) {

}

void Flip::seaThermalTransfer(CPU_Particle* particle) {
	if (particle->position.y <= -HALF_SIZE.y * 0.25f) {
		const vec1 t = f_map(-HALF_SIZE.y, -HALF_SIZE.y * 0.25f, 1.0f, 0.0f, clamp(particle->position.y, -HALF_SIZE.y, -HALF_SIZE.y * 0.25f));
		const vec1 temp_diff = SEA_SURFACE_TEMP - particle->temperature;
		particle->temperature += temp_diff * t * DT;
	}
}

void Flip::atmosphereThermalTransfer(CPU_Particle* particle) {
	if (particle->position.y >= HALF_SIZE.y * 0.25f) {
		const vec1 t = f_map(HALF_SIZE.y, HALF_SIZE.y * 0.25f, 1.0f, 0.0f, clamp(particle->position.y, HALF_SIZE.y * 0.52f, HALF_SIZE.y));
		const vec1 temp_diff = ATMOSPHERE_TEMP - particle->temperature;
		particle->temperature += temp_diff * t * DT;
	}
}

void Flip::scatter() {
	for (CPU_Cell& cell : grid) {
		cell.type = Cell_Type::AIR;
		cell.particle_count = 0;
		cell.particles.clear();
		for (CPU_Particle& particle : particles) {
			if (insideAABB(particle.position, cell.pmin, cell.pmax)) {
				cell.particle_count++;
				cell.particles.push_back(&particle);
			}
		}
		if (cell.particle_count > 0) {
			cell.type = Cell_Type::FLUID;
		}
		cell.density = sampleDensity(&cell);// u_to_f(cell.particle_count) / 10.0f;
	}
	for (CPU_Cell& cell : grid) {
		for (CPU_Particle* particle: cell.particles) {
			vec1 temp_diff = particle->temperature - cell.temperature;
			vec1 weight = calculateInterpolationWeight(particle, &cell) / u_to_f(cell.particle_count);
			weight *= CELL_HEAT_GAIN * DT;

			cell.temperature += temp_diff * weight;
		}
	}
}

void Flip::gather() {

}

void Flip::computeGrid() {
	vector<CPU_Cell> new_grid = grid;
	// Advection
	for (uint x = 0; x < GRID_CELLS.x; ++x) {
		for (uint y = 0; y < GRID_CELLS.y; ++y) {
			for (uint z = 0; z < GRID_CELLS.z; ++z) {
				CPU_Cell* this_cell = getGrid(x, y, z);
				CPU_Cell* cell = getGrid(new_grid, x, y, z);

				const vec1 ambient_temp_diff = AMBIENT_TEMP - cell->temperature;
				const vec1 ambient_heat_transfer = CELL_AMBIENT_HEAT_TRANSFER * ambient_temp_diff * DT;
				cell->temperature += ambient_heat_transfer;

				//for (int dx = -1; dx <= 1; dx++) {
				//	for (int dy = -1; dy <= 1; dy++) {
				//		for (int dz = -1; dz <= 1; dz++) {
				//			if (dx == 0 && dy == 0 && dz == 0) {
				//				continue;
				//			}
				//			const uint ddx = x + dx;
				//			const uint ddy = y + dy;
				//			const uint ddz = z + dz;
				//			if (ddx >= 0 && ddx < GRID_CELLS.x && ddy >= 0 && ddy < GRID_CELLS.y && ddz >= 0 && ddz < GRID_CELLS.z) {
				//				CPU_Cell* neighbor = getGrid(ddx, ddy, ddx);
				//
				//			}
				//		}
				//	}
				//}
			}
		}
	}
	grid = new_grid;
}

void Flip::computeParticles() {
	for (CPU_Particle& particle : particles) {
		vec1 temp_diff = particle.cell->temperature - particle.temperature;
	}
}

CPU_Cell* Flip::getGrid(const uint64& x, const uint64& y, const uint64& z) {
	return &grid[x * (GRID_CELLS.y * GRID_CELLS.z) + y * GRID_CELLS.z + z];
}

CPU_Cell* Flip::getGrid(vector<CPU_Cell>& in_grid, const uint64& x, const uint64& y, const uint64& z) {
	return &in_grid[x * (GRID_CELLS.y * GRID_CELLS.z) + y * GRID_CELLS.z + z];
}

vector<GPU_Particle> Flip::gpuParticles() const {
	vector<GPU_Particle> gpu;
	for (const CPU_Particle& particle : particles) {
		gpu.push_back(GPU_Particle(particle));
	}
	return gpu;
}

vector<GPU_Cell> Flip::gpuGrid() const {
	vector<GPU_Cell> gpu;

	for (uint64 x = 0; x < GRID_CELLS.x; ++x) {
		for (uint64 y = 0; y < GRID_CELLS.y; ++y) {
			for (uint64 z = 0; z < GRID_CELLS.z; ++z) {
				const uint64 index = x * (GRID_CELLS.y * GRID_CELLS.z) + y * GRID_CELLS.z + z;
				gpu.push_back(GPU_Cell(grid[index]));
			}
		}
	}
	return gpu;
}

void Flip::particleCollisions() {
	for (uint i = 0; i < SAMPLES; i++) {
		for (uint x = 0; x < GRID_CELLS.x; ++x) {
			for (uint y = 0; y < GRID_CELLS.y; ++y) {
				for (uint z = 0; z < GRID_CELLS.z; ++z) {
					CPU_Cell* cell = getGrid(x,y,z);
					cell->particles.clear();
				}
			}
		}
		for (CPU_Particle& particle : particles) {
			for (uint x = 0; x < GRID_CELLS.x; ++x) {
				for (uint y = 0; y < GRID_CELLS.y; ++y) {
					for (uint z = 0; z < GRID_CELLS.z; ++z) {
						CPU_Cell* cell = getGrid(x,y,z);
						if (insideAABB(particle.position, cell->pmin, cell->pmax)) {
							cell->particles.push_back(&particle);
							particle.cell = cell;
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
							neighbors.insert(neighbors.end(), getGrid(x, y, z)->particles.begin(), getGrid(x, y, z)->particles.end());
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
			boundingCollisions(&particle);
		}
	}
}

void Flip::particleCollisionsUnoptimized() {
	for (uint i = 0; i < SAMPLES; i++) {
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
		boundingCollisions(&particle);
	}
}

void Flip::boundingCollisions(CPU_Particle* particle) {
	particle->colliding = false;
	if (particle->position.x - PARTICLE_RADIUS < 0.0 - HALF_SIZE.x) {
		particle->position.x = (PARTICLE_RADIUS - HALF_SIZE.x);
		particle->velocity.x = (-particle->velocity.x * RESTITUTION);
		particle->colliding = true;
	}
	else if (particle->position.x + PARTICLE_RADIUS >  HALF_SIZE.x ) {
		particle->position.x = (HALF_SIZE.x - PARTICLE_RADIUS);
		particle->velocity.x = (-particle->velocity.x * RESTITUTION);
		particle->colliding = true;
	}

	if (particle->position.y - PARTICLE_RADIUS < 0.0 - HALF_SIZE.y) {
		particle->position.y = (PARTICLE_RADIUS - HALF_SIZE.y);
		particle->velocity.y = (-particle->velocity.y * RESTITUTION);
		particle->colliding = true;
	}
	else if (particle->position.y + PARTICLE_RADIUS >  HALF_SIZE.y ) {
		particle->position.y = (HALF_SIZE.y - PARTICLE_RADIUS);
		particle->velocity.y = (-particle->velocity.y * RESTITUTION);
		particle->colliding = true;
	}

	if (particle->position.z - PARTICLE_RADIUS < 0.0 - HALF_SIZE.z) {
		particle->position.z = (PARTICLE_RADIUS - HALF_SIZE.z);
		particle->velocity.z = (-particle->velocity.z * RESTITUTION);
		particle->colliding = true;
	}
	else if (particle->position.z + PARTICLE_RADIUS >  HALF_SIZE.z ) {
		particle->position.z = (HALF_SIZE.z - PARTICLE_RADIUS);
		particle->velocity.z = (-particle->velocity.z * RESTITUTION);
		particle->colliding = true;
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
	const vec3 collision_normal = glm::normalize(particle_b->position - particle_a->position);
	const vec3 relative_velocity = particle_b->velocity - particle_a->velocity;
	const vec1 velocity_along_normal = glm::dot(relative_velocity, collision_normal);

	if (velocity_along_normal > 0.0f) {
		return;
	}

	const vec1 impulse_scalar = (-(1.0f + PARTICLE_RESTITUTION) * velocity_along_normal) / (1.0f / particle_a->mass + 1.0f / particle_b->mass);
	const vec3 impulse = impulse_scalar * collision_normal;
	particle_a->velocity -= impulse / particle_a->mass;
	particle_b->velocity += impulse / particle_b->mass;
}

vec1 Flip::sampleDensity(const CPU_Cell* cell) const {
	vec1 density = 0.0f;

	for (const CPU_Cell* neighbor : cell->neighbors) {
		if (neighbor) {
			for (const CPU_Particle* particle : neighbor->particles) {
				const vec1 distance = glm::length(particle->position - cell->center);
				const vec1 weight = smoothWeight(distance);
				density += particle->mass * weight;
			}
		}
	}
	return density;
}

vec1 Flip::sampleTemperature(const CPU_Cell* cell) const {
	vec1 temperature = 0.0f;

	for (const CPU_Cell* neighbor : cell->neighbors) {
		if (neighbor) {
			for (const CPU_Particle* particle : neighbor->particles) {
				const vec1 distance = glm::length(particle->position - cell->center);
				const vec1 weight = smoothWeight(distance);
				temperature += particle->temperature * weight;
			}
		}
	}
	return temperature;
}

vec1 calculateAirDensity(const vec1& pressure, const vec1& temperature) {
	return pressure / (AIR_GAS_CONSTANT * temperature);
}

vec1 calculateInterpolationWeight(const CPU_Particle* particle, const CPU_Cell* cell) {
	const vec3 cell_center = (cell->pmin + cell->pmax) * 0.5f;
	const vec3 diff = particle->position - cell_center;
	const vec1 distance = glm::length(diff);

	const vec1 max_distance = glm::length(cell->pmax - cell->pmin) * 0.5f;
	const vec1 weight = std::max(0.0f, 1.0f - distance / max_distance);
	return 1.0f;
	return weight;
}

vec1 Flip::smoothWeight(const vec1& distance) const {
	//vec1 value = glm::max(0.0f, radius * radius - distance * distance);
	//return value * value * value
	const vec1 value = glm::max(0.0f, SMOOTH_RADIUS - distance);
	const vec1 volume = glm::pi<vec1>() * pow(SMOOTH_RADIUS, 3.0f) /3.0f;
	return value / volume;
}