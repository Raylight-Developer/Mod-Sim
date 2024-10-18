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
	GPU_Particle(const CPU_Particle& particle);
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

	GPU_Cell();
	GPU_Cell(const CPU_Cell& cell);
};

using Grid = vector<vector<vector<CPU_Cell>>>;
using Particles = vector<CPU_Particle>;

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

	Grid grid;
	Particles particles;

	Flip();

	void init();
	void initGrid();
	void initParticles();

	void simulate(const dvec1& delta_time);

	void integrate(const dvec1& delta_time);

	void thermodynamics(CPU_Particle& particle, const dvec1& delta_time);
	void seaThermalTransfer(CPU_Particle& particle, const dvec1& delta_time);
	void atmosphereThermalTransfer(CPU_Particle& particle, const dvec1& delta_time);

	void scatter(const dvec1& delta_time);
	void gather(const dvec1& delta_time);

	void navierStokes();
	void computeCoriolis();
	void computePressure();
	void computeTemperature();




	void particleCollisions(const dvec1& delta_time);
	void particleCollisionsUnoptimized(const dvec1& delta_time);
	void boundingCollisions(CPU_Particle& particle);

	bool resolveOverlap(CPU_Particle* particle_a, CPU_Particle* particle_b);
	void resolveCollision(CPU_Particle* particle_a, CPU_Particle* particle_b);
};