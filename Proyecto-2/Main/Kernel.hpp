#pragma once

#include "Shared.hpp"

#include "OpenGl.hpp"

#include "Particle.hpp"
#include "Bvh.hpp"

enum struct Texture_Field;

struct Kernel {
	vec1 PARTICLE_RADIUS;
	uint PARTICLE_COUNT;
	uint MAX_OCTREE_DEPTH;
	vec1 POLE_BIAS;
	vec1 POLE_BIAS_POWER;
	vec2 POLE_GEOLOCATION;
	vec1 EARTH_TILT;
	vec1 YEAR_TIME;
	vec1 DAY_TIME;
	vec1 TIME_SCALE;
	int CALENDAR_MONTH;
	int CALENDAR_DAY;
	int CALENDAR_HOUR;
	int CALENDAR_MINUTE;
	uint DAY;

	vec1 DT;
	uint RUNFRAME;
	uint SAMPLES;
	vec1 SDT;

	vec3 sun_dir;

	vector<CPU_Particle> particles;
	unordered_map<Texture_Field, Texture> textures;

	vector<GPU_Particle> gpu_particles;
	vector<GPU_Bvh> bvh_nodes;

	vec1  time;
	uint  frame_count;

	Kernel();

	void preInit();
	void preInitParticles();
	void traceInitProperties(CPU_Particle* particle) const;

	void init();
	void initParticles();
	void buildBvh();

	void simulate(const dvec1& delta_time);
	void updateTime();
	void rotateEarth(CPU_Particle* particle) const;
	void calculateSPH(CPU_Particle* particle) const;
	void calculateSunlight(CPU_Particle* particle) const;
	void calculateThermodynamics(CPU_Particle* particle) const;

	vec3 sunDir() const;
	void calculateDate();
	void calculateDateTime();
	void calculateYearTime();
	void calculateDayTime();
	vec3 rotateGeoloc(const vec3& point, const vec2& geoloc) const;
};

