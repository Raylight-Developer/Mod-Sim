#pragma once

#include "Shared.hpp"

#include "OpenGl.hpp"

#include "Particle.hpp"
#include "Bvh.hpp"

enum struct Texture_Field;

struct Kernel {
	vec1  PROBE_RADIUS;
	uint  PROBE_COUNT;
	uint  PROBE_MAX_OCTREE_DEPTH;
	dvec1 PROBE_POLE_BIAS;
	dvec1 PROBE_POLE_BIAS_POWER;
	dvec2 PROBE_POLE_GEOLOCATION;

	vec1  PARTICLE_RADIUS;
	uint  PARTICLE_COUNT;
	uint  PARTICLE_MAX_OCTREE_DEPTH;
	dvec1 PARTICLE_POLE_BIAS;
	dvec1 PARTICLE_POLE_BIAS_POWER;
	dvec2 PARTICLE_POLE_GEOLOCATION;

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
	dvec1 SDT;
	uint  RUNFRAME;
	uint  SUB_SAMPLES;

	dvec3 sun_dir;

	unordered_map<Texture_Field, Texture> textures;

	vector<CPU_Probe>        probes;
	vector<CPU_Particle>     particles;

	vector<GPU_Probe>        gpu_probes;
	vector<GPU_Particle>     gpu_particles;

	vector<GPU_Bvh>          probe_nodes;
	vector<GPU_Bvh>          particle_nodes;

	vector<Compute_Probe>    compute_probes;
	vector<Compute_Particle> compute_particles;
	GLuint compute_program;

	Kernel();

	void traceInitProperties(CPU_Probe* probe) const;

	void updateGPUProbes();
	void buildProbes();

	void updateGPUParticles();
	void buildParticles();

	void lock();
	void lockProbes();
	void lockParticles();

	void simulate(const dvec1& delta_time);
	void updateTime();
	void updateProbePosition(CPU_Probe* probe) const;
	void calculateSunlight(CPU_Probe* probe) const;

	void scatterSPH(CPU_Probe* probe) const;
	void scatterWind(CPU_Probe* probe) const;
	void gatherWind(CPU_Probe* probe) const;
	void gatherThermodynamics(CPU_Probe* probe) const;

	void particleCompute();
	void calculateParticle(CPU_Particle* particle) const;
	void updateParticlePosition(CPU_Particle* particle) const;

	dvec3 sunDir() const;
	void calculateDate();
	void calculateDateTime();
	void calculateYearTime();
	void calculateDayTime();
	dvec3 rotateGeoloc(const dvec3& point, const dvec2& geoloc) const;
	dquat rotateGeoloc(const dvec2& geoloc) const;
};