#pragma once

#include "Shared.hpp"
#include "Particle.hpp"

struct CPU_Bvh;

struct CPU_Bvh {
	vec3 p_min;
	vec3 p_max;
	CPU_Bvh* parent;
	vector<CPU_Probe> probes;
	vector<CPU_Particle> particles;
	vector<CPU_Bvh> children;
	uint item_count;
	bool discard;

	CPU_Bvh();
	CPU_Bvh(const vec3& pmin, const vec3& pmax);

	void growToInclude(const vec3& min, const vec3& max);
	vec3 getSize() const;
	vec3 getCenter() const;
	void growToInclude(const CPU_Probe& probe, const vec1& radius);
	bool contains(const CPU_Probe& probe);
	void growToInclude(const CPU_Particle& particle, const vec1& radius);
	bool contains(const CPU_Particle& particle);
	void split();
	bool operator==(const CPU_Bvh& other) const;
};

struct alignas(16) GPU_Bvh {
	vec3  p_min;
	uint  start;
	vec3  p_max;
	uint  end;
	ivec4 pointers_a;
	ivec4 pointers_b;

	GPU_Bvh();
};

struct Bvh_Particle {
	vec3   p_min;
	vec3   p_max;
	vec3   center;
	uint64 index;
	CPU_Probe part;
	Bvh_Particle(const vec3& p_min, const vec3& p_max, const vec3& center, const uint64& index, const CPU_Probe& part);
};

struct Builder {
	vector<CPU_Probe> probes;
	CPU_Bvh root_node;
	uint depth_cutoff;
	vec1 particle_radius;


	GPU_Bvh gpu_root_node;
	vector<GPU_Bvh> nodes;

	Builder(const vector<CPU_Probe>& probes, const vec1& probe_radius, const uint& max_depth);

	void splitBvh(CPU_Bvh* node, const uint& depth);
	uint convertBvh(CPU_Bvh* node);
	void splitBvhSPH(CPU_Bvh* node, const uint& depth);
	void growBvhSPH(CPU_Bvh* node);
	uint convertBvhSPH(CPU_Bvh* node);
};

struct Particle_Builder {
	vector<CPU_Particle> particles;
	CPU_Bvh root_node;
	uint depth_cutoff;
	vec1 particle_radius;

	GPU_Bvh gpu_root_node;
	vector<GPU_Bvh> nodes;

	Particle_Builder(const vector<CPU_Particle>& particles, const vec1& particle_radius, const uint& max_depth);

	void splitBvh(CPU_Bvh* node, const uint& depth);
	uint convertBvh(CPU_Bvh* node);
};