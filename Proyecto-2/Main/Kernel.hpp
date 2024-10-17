#pragma once

#include "Shared.hpp"

struct CPU_Particle {
	dvec1 mass;
	dvec1 humidity;
	dvec1 pressure;
	dvec1 temperature;

	dvec3 position;
	dvec3 velocity;
	dvec3 acceleration;

	bool  colliding;

	CPU_Particle();
};

struct alignas(16) GPU_Particle {
	vec4 position;
	vec4 velocity;

	GPU_Particle();
	GPU_Particle(const CPU_Particle& particle);
};

struct CPU_Cell {
	dvec1 density;
	dvec1 humidity;
	dvec1 pressure;
	dvec1 temperature;
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

// Particles
void  initialize(vector<CPU_Particle>& points);
void  simulate  (vector<CPU_Particle>& points, const dvec1& delta_time);
void updateVelocity(CPU_Particle& particle, const vector<CPU_Particle>& neighbors, const dvec1& delta_time);
void updatePosition(CPU_Particle& particle, const dvec1& delta_time);
void handleBorderCollision(CPU_Particle& particle);
dvec3 computeNavierStokes(const CPU_Particle& particle, const vector<CPU_Particle>& neighbors);
dvec3 computeCoriolisEffect(const CPU_Particle& particle);
dvec1 computeThermodynamics(CPU_Particle& particle);

//Grid
void initialize(Grid& grid);
void simulate  (Grid& grid, const dvec1& delta_time);
dvec3 computePressureGradient(const Grid& grid, const uint64& x, const uint64& y, const uint64& z, const uvec3& size);

void advection (Grid& grid, const dvec1& delta_time);
void diffusion (Grid& grid, const dvec1& delta_time);
void projection(Grid& grid, const dvec1& delta_time);