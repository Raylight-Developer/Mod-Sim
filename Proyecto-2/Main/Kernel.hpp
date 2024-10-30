#pragma once

#include "Shared.hpp"

#include "OpenGl.hpp"

#include "Particle.hpp"
#include "Bvh.hpp"

enum struct Texture_Field;

struct Kernel {
	vec1  PARTICLE_RADIUS;
	uint  PARTICLE_COUNT;
	uint  MAX_OCTREE_DEPTH;
	dvec1 POLE_BIAS;
	dvec1 POLE_BIAS_POWER;
	dvec2 POLE_GEOLOCATION;
	dvec1 EARTH_TILT;
	dvec1 YEAR_TIME;
	dvec1 DAY_TIME;
	dvec1 TIME_SCALE;
	int   CALENDAR_MONTH;
	int   CALENDAR_DAY;
	int   CALENDAR_HOUR;
	int   CALENDAR_MINUTE;
	uint  DAY;
	bool  BVH_SPH;

	dvec1 DT;
	uint  RUNFRAME;
	uint  SAMPLES;
	dvec1 SDT;

	dvec3 sun_dir;

	vector<CPU_Particle> particles;
	unordered_map<Texture_Field, Texture> textures;
	Texture sph_texture;

	vector<GPU_Particle> gpu_particles;
	vector<GPU_Bvh> bvh_nodes;

	dvec1 time;
	uint  frame_count;

	Kernel();

	void buildBvh();
	void buildParticles();
	void traceInitProperties(CPU_Particle* particle) const;

	void lock();
	void lockParticles();
	void generateSPHTexture();

	void simulate(const dvec1& delta_time);
	void updateTime();
	void rotateEarth(CPU_Particle* particle) const;
	void calculateSunlight(CPU_Particle* particle) const;

	void scatterSPH(CPU_Particle* particle) const;
	void scatterPressure(CPU_Particle* particle) const;
	void gatherPressure(CPU_Particle* particle) const;
	void gatherThermodynamics(CPU_Particle* particle) const;

	dvec3 sunDir() const;
	void calculateDate();
	void calculateDateTime();
	void calculateYearTime();
	void calculateDayTime();
	dvec3 rotateGeoloc(const dvec3& point, const dvec2& geoloc) const;
};