#pragma once

#include "Shared.hpp"

enum struct Cell_Type {
	AIR_CELL,
	FLUID_CELL,
	SOLID_CELL
};

struct Scene {
	dvec1 gravity = -9.81;
	dvec1 dt = 1.0 / 120.0;
	dvec1 flipRatio = 0.9;
	uint numPressureIters = 100;
	uint numParticleIters = 2;
	uint frameNr = 0;
	dvec1 overRelaxation = 1.9;
	bool compensateDrift = true;
	bool separateParticles = true;
	bool paused = true;
	bool showObstacle = true;
	dvec1 obstacleVelX = 0.0;
	dvec1 obstacleVelY = 0.0;
	bool showParticles = true;
	bool showGrid = false;
};

struct FlipFluid {
	dvec1 density;
	int fNumX;
	int fNumY;
	dvec1 h;
	dvec1 fInvSpacing;
	uint fNumCells;

	vector<dvec1> u;
	vector<dvec1> v;
	vector<dvec1> du;
	vector<dvec1> dv;
	vector<dvec1> prevU;
	vector<dvec1> prevV;
	vector<dvec1> p;;
	vector<dvec1> s;
	vector<Cell_Type> cellType;
	vector<dvec3> cellColor;

	// particles

	uint maxParticles;

	vector<dvec2> particlePos;
	vector<dvec3> particleColor;

	vector<dvec2> particleVel;
	vector<dvec1> particleDensity;
	dvec1 particleRestDensity;

	dvec1 particleRadius;
	dvec1 pInvSpacing;
	dvec1 pNumX;
	dvec1 pNumY;
	dvec1 pNumCells;

	vector<uint> firstCellParticle;
	vector<uint> numCellParticles;
	vector<uint> cellParticleIds;

	uint numParticles;

	Scene scene;

	FlipFluid();

	void integrateParticles(dvec1 dt, dvec1 gravity);
	void pushParticlesApart(uint numIters);
	void handleParticleCollisions();
	void updateParticleDensity();
	void transferVelocities(bool toGrid, dvec1 flipRatio);
	void solveIncompressibility(uint numIters, dvec1 dt, dvec1 overRelaxation, bool compensateDrift = true);
	void updateParticleColors();
	void setSciColor(uint cellNr, dvec1 val, dvec1 minVal, dvec1 maxVal);
	void updateCellColors();
	void simulate(dvec1 dt);
};