#pragma once

#include "Shared.hpp"
#include "Fluid.hpp"

struct CPU_Cell;

struct CPU_Particle {
	vec1 mass;
	vec1 density;
	vec1 temperature;

	vec3 position;
	vec3 velocity;
	vec3 acceleration;

	bool  colliding;

	uvec3 cell_id;
	CPU_Cell* cell;

	CPU_Particle();
};

struct alignas(16) GPU_Particle {
	vec3 position;
	vec1 temperature;
	vec4 velocity;

	GPU_Particle();
	GPU_Particle(const CPU_Particle& particle);
};

enum struct Cell_Type {
	AIR,
	FLUID
};

struct CPU_Cell {
	vec1 density;
	vec1 humidity;
	vec1 pressure;
	vec1 temperature;

	vec3 pmin;
	vec3 pmax;
	vec3 center;

	vec3 velocity_field;
	Cell_Type type;

	vector<CPU_Particle*> particles;
	uint  particle_count;

	array<CPU_Cell*, 26> neighbors;

	CPU_Cell();
};

struct GPU_Cell {
	vec1 density;
	vec1 humidity;
	vec1 pressure;
	vec1 temperature;

	vec3 pmin;
	uint a;
	vec3 pmax;
	uint b;

	GPU_Cell();
	GPU_Cell(const CPU_Cell& cell);
};

struct Flip {
	vec1  PARTICLE_RADIUS;
	vec1  PARTICLE_AREA;
	uint  PARTICLE_COUNT;
	uvec3 GRID_CELLS;
	uint  GRID_COUNT;
	vec1  CELL_SIZE;
	vec1  INV_CELL_SIZE;
	vec3  GRID_SIZE;
	vec3  HALF_SIZE;
	vec1  REST_DENSITY;
	vec1  SMOOTH_RADIUS;
	vec1  DT;
	uint  RUNFRAME;
	uint  SAMPLES;
	vec1  SDT;

	vector<CPU_Cell> grid;
	vector<CPU_Particle> particles;
	vector<CPU_Cell> temp_grid;

	Flip();

	void init(const vec1& PARTICLE_RADIUS, const uint& PARTICLE_COUNT, const uvec3& GRID_CELLS);
	void initGrid();
	void initParticles();

	void simulate(const dvec1& delta_time);

	void integrate();

	void thermodynamics(CPU_Particle* particle);
	void seaThermalTransfer(CPU_Particle* particle);
	void atmosphereThermalTransfer(CPU_Particle* particle);

	void scatter();
	void gather();

	void computeGrid();
	void computeParticles();

	CPU_Cell* getGrid(const uint64& x, const uint64& y, const uint64& z);
	CPU_Cell* getGrid(vector<CPU_Cell>& grid, const uint64& x, const uint64& y, const uint64& z);

	vector<GPU_Particle> gpuParticles() const;
	vector<GPU_Cell> gpuGrid() const;

	void particleCollisions();
	void particleCollisionsUnoptimized();
	void boundingCollisions(CPU_Particle* particle);

	bool resolveOverlap(CPU_Particle* particle_a, CPU_Particle* particle_b);
	void resolveCollision(CPU_Particle* particle_a, CPU_Particle* particle_b);

	vec1 sampleDensity(const CPU_Cell* cell) const;
	vec1 sampleTemperature(const CPU_Cell* cell) const;
	vec1 sampleTemperature(const CPU_Particle* particle) const;

	vec1 smoothWeight(const vec1& distance) const;
};

vec1 calculateAirDensity(const vec1& pressure, const vec1& temperature);
vec1 calculateInterpolationWeight(const CPU_Particle* particle, const CPU_Cell* cell);