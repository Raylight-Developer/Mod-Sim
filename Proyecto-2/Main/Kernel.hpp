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
using Cloud = vector<CPU_Particle>;

// Particles
void initialize(Cloud& points);
void simulate  (Cloud& points, const dvec1& delta_time);
void updateVelocity(CPU_Particle& particle, const Cloud& neighbors, const dvec1& delta_time);
void updatePosition(CPU_Particle& particle, const dvec1& delta_time);
void handleBorderCollision(CPU_Particle& particle);
dvec3 computeNavierStokes(const CPU_Particle& particle, const Cloud& neighbors);
dvec3 computeCoriolisEffect(const CPU_Particle& particle);
void  computeThermodynamics(CPU_Particle& particle, const dvec1& half_size);

//Grid
void initialize(Grid& grid);
void simulate  (Grid& grid, const Cloud& particles, const dvec1& delta_time);
void integrate (CPU_Cell& cell, const dvec1& delta_time);
void computeParticleData(CPU_Cell& cell, const Cloud& particles, const dvec1& delta_time, const dvec1& normalized_density);
dvec3 computePressureGradient(const Grid& grid, const uint64& x, const uint64& y, const uint64& z, const uvec3& size);

void  computeConvection(CPU_Cell& cell, const dvec1& delta_time);

void advection (Grid& grid, const dvec1& delta_time);
void diffusion (Grid& grid, const dvec1& delta_time);
void projection(Grid& grid, const dvec1& delta_time);