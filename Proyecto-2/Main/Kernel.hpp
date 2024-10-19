#pragma once

#include "Shared.hpp"
struct CPU_Cell;

struct CPU_Particle {
	dvec1 mass;
	dvec1 density;
	dvec1 humidity;
	dvec1 pressure;
	dvec1 temperature;

	dvec3 position;
	dvec3 velocity;
	dvec3 acceleration;

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
	GPU_Particle(const CPU_Particle* particle);
};

struct CPU_Cell {
	dvec1 density;
	dvec1 humidity;
	dvec1 pressure;
	dvec1 temperature;

	vector<CPU_Particle*> particles;
	uint  particle_count;

	dvec3 pmin;
	dvec3 pmax;

	dvec3 velocity_field;
	dvec3 acceleration_field;

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
	GPU_Cell(const CPU_Cell* cell);
};

struct GPU_Octree {
	uvec4 pointers_a;
	uvec4 pointers_b;
	vec3 pmin;
	uint leaf;
	vec3 pmax;
	uint pad;
};

struct Flip {
	dvec1 PARTICLE_RADIUS;
	uint  PARTICLE_COUNT;
	uvec3 GRID_CELLS;
	uint  GRID_COUNT;
	dvec1 CELL_SIZE;
	dvec1 INV_CELL_SIZE;
	dvec3 GRID_SIZE;
	dvec3 HALF_SIZE;
	dvec1 REST_DENSITY;
	dvec1 DT;
	uint  SAMPLES;
	dvec1 SDT;

	vector<CPU_Cell*> grid;
	vector<CPU_Particle*> particles;

	Flip();

	void init();
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

	void navierStokes();
	void computeCoriolis();
	void computePressure();
	void computeTemperature();

	CPU_Cell* getGrid(const uint64& x, const uint64& y, const uint64& z);

	vector<GPU_Particle> gpuParticles() const;
	vector<GPU_Cell> gpuGrid() const;

	void particleCollisions();
	void particleCollisionsUnoptimized();
	void boundingCollisions(CPU_Particle* particle);

	bool resolveOverlap(CPU_Particle* particle_a, CPU_Particle* particle_b);
	void resolveCollision(CPU_Particle* particle_a, CPU_Particle* particle_b);
};

dvec1 calculateAirDensity(const dvec1& pressure, const dvec1& temperature);
dvec1 calculateInterpolationWeight(const CPU_Particle* particle, const CPU_Cell* cell);