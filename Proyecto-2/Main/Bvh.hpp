#pragma once

#include "Shared.hpp"
#include "Particle.hpp"

struct alignas(16) GPU_Bvh {
	vec3 p_min;
	uint particle_count;
	vec3 p_max;
	uint pointer_particle;
	uint pointer_a;
	uint pointer_b;
	uvec2 padding = uvec2(0);

	GPU_Bvh(const vec3& p_min = vec3(MAX_VEC1), const vec3& p_max = vec3(MIN_VEC1), const uint& pointer_a = 0U, const uint& pointer_b = 0U, const uint& pointer_particle = 0U, const uint& particle_count = 0U );

	void growToInclude(const vec3& min, const vec3& max);
	vec3 getSize();
	vec3 getCenter();
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
	vector<Bvh_Particle> bvh_particles;
	vector<GPU_Bvh> node_list;
	GPU_Bvh root_node;

	Builder(const vector<CPU_Particle>& particles, const vec1& particle_radius, const uint& depth);

	void splitBvh(const uint& parentIndex, const uint& particleGlobalStart, const uint& triNum, const uint& depth);
	void splitAxis(const GPU_Bvh& node, const uint& start, const uint& count, uint8& axis, vec1& pos, vec1& cost) const;
	float splitEval(const uint8& splitAxis, const vec1& splitPos, const uint& start, const uint& count) const;
	static float nodeCost(const vec3& size, const uint& numParticles);
};