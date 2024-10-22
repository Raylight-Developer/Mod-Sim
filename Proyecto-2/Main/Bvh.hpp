#pragma once

#include "Shared.hpp"
#include "Particle.hpp"

struct CPU_Bvh;

struct CPU_Bvh {
	vec3 p_min;
	vec3 p_max;
	vector<CPU_Particle> particles;
	vector<CPU_Bvh> children;
	bool discard;

	CPU_Bvh();
	CPU_Bvh(const vec3& pmin, const vec3& pmax);

	void growToInclude(const CPU_Particle& particle, const vec1& radius);
	void growToInclude(const vec3& min, const vec3& max);
	vec3 getSize() const;
	vec3 getCenter() const;
	bool contains(const CPU_Particle& particle);
	void split();
	bool operator==(const CPU_Bvh& other) const;
};

struct alignas(16) GPU_Bvh {
	vec3 p_min;
	uint particle_count;
	vec3 p_max;
	uint particle_pointer;
	ivec4 pointers_a;
	ivec4 pointers_b;

	GPU_Bvh();
};

struct Bvh_Particle {
	vec3   p_min;
	vec3   p_max;
	vec3   center;
	uint64 index;
	CPU_Particle part;
	Bvh_Particle(const vec3& p_min, const vec3& p_max, const vec3& center, const uint64& index, const CPU_Particle& part);
};

struct Builder {
	vector<CPU_Particle> particles;
	CPU_Bvh root_node;
	uint max_depth;
	vec1 particle_radius;


	GPU_Bvh gpu_root_node;
	vector<GPU_Bvh> nodes;

	Builder(const vector<CPU_Particle>& particles, const vec1& particle_radius, const uint& depth);

	void splitBvh(CPU_Bvh* parent, const uint& depth);
	void convertBvh(CPU_Bvh* parent);
};