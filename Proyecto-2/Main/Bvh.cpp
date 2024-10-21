#include "Bvh.hpp"

#include "Kernel.hpp"

GPU_Bvh::GPU_Bvh(const vec3& p_min, const vec3& p_max, const uint& pointer_a, const uint& pointer_b, const uint& pointer_particle, const uint& particle_count) :
	p_min(p_min),
	p_max(p_max),
	pointer_a(pointer_a),
	pointer_b(pointer_b),
	pointer_particle(pointer_particle),
	particle_count(particle_count)
{}

void GPU_Bvh::growToInclude(const vec3& min, const vec3& max) {
	p_min.x = min.x < p_min.x ? min.x : p_min.x;
	p_min.y = min.y < p_min.y ? min.y : p_min.y;
	p_min.z = min.z < p_min.z ? min.z : p_min.z;
	p_max.x = max.x > p_max.x ? max.x : p_max.x;
	p_max.y = max.y > p_max.y ? max.y : p_max.y;
	p_max.z = max.z > p_max.z ? max.z : p_max.z;
}

vec3 GPU_Bvh::getSize() {
	return p_max - p_min;
}

vec3 GPU_Bvh::getCenter() {
	return (p_min + p_max) / 2.0f;
}


Builder::Builder(const vector<CPU_Particle>& particles, const vec1& particle_radius, const uint& depth) :
	particles(particles)
{
	for (uint i = 0; i < len32(particles); i++) {
		const vec3 center = (particles[i].position);
		const vec3 max = particles[i].position + particle_radius;
		const vec3 min = particles[i].position - particle_radius;
		bvh_particles.push_back(Bvh_Particle(min, max, center, i, particles[i]));
		root_node.growToInclude(min, max);
	}

	node_list.push_back(root_node);
	splitBvh(0, 0, len32(particles), depth);

	this->particles.clear();
	for (uint i = 0; i < len32(bvh_particles); i++) {
		this->particles.push_back(bvh_particles[i].part);
	}
}

void Builder::splitBvh(const uint& parentIndex, const uint& triGlobalStart, const uint& triNum, const uint& depth) {
	GPU_Bvh& parent = node_list[parentIndex];
	vec3 size = parent.getSize();
	vec1 parentCost = nodeCost(size, triNum);

	uint8 split_axis;
	vec1 splitPos;
	vec1 cost;

	splitAxis(parent, triGlobalStart, triNum, split_axis, splitPos, cost);

	if (cost < parentCost && depth > 0) {
		GPU_Bvh boundsLeft;
		GPU_Bvh boundsRight;
		uint numOnLeft = 0;

		for (uint i = triGlobalStart; i < triGlobalStart + triNum; i++) {
			Bvh_Particle part = bvh_particles[i];
			if (part.center[split_axis] < splitPos) {
				boundsLeft.growToInclude(part.p_min, part.p_max);

				Bvh_Particle swap = bvh_particles[triGlobalStart + numOnLeft];
				bvh_particles[triGlobalStart + numOnLeft] = part;
				bvh_particles[i] = swap;
				numOnLeft++;
			}
			else {
				boundsRight.growToInclude(part.p_min, part.p_max);
			}
		}

		uint numOnRight = triNum - numOnLeft;
		uint triStartLeft = triGlobalStart;
		uint triStartRight = triGlobalStart + numOnLeft;

		node_list.push_back(GPU_Bvh(boundsLeft.p_min, boundsLeft.p_max, triStartLeft));
		uint childIndexRight = len32(node_list);
		uint childIndexLeft = childIndexRight - 1;
		node_list.push_back(GPU_Bvh(boundsRight.p_min, boundsRight.p_max, triStartRight));

		parent.pointer_a = childIndexLeft;
		parent.pointer_b = childIndexRight;

		splitBvh(childIndexLeft, triGlobalStart, numOnLeft, depth - 1);
		splitBvh(childIndexRight, triGlobalStart + numOnLeft, numOnRight, depth - 1);
	}
	else {
		parent.pointer_a = 0;
		parent.pointer_b = 0;
		parent.pointer_particle = triGlobalStart;
		parent.particle_count = triNum;
	}
}

void Builder::splitAxis(const GPU_Bvh& node, const uint& start, const uint& count, uint8& axis, vec1& pos, vec1& cost) const {
	if (count <= 1) {
		axis = 0;
		pos = 0.0f;
		cost = MAX_VEC1;
		return;
	}

	vec1 bestSplitPos = 0;
	uint8 bestSplitAxis = 0;
	const uint8 numSplitTests = 5;

	vec1 bestCost = MAX_VEC1;

	for (uint8 axis = 0; axis < 3; axis++) {
		for (uint8 i = 0; i < numSplitTests; i++) {
			vec1 splitT = (u_to_f(i) + 1.0f) / (u_to_f(numSplitTests) + 1.0f);
			vec1 splitPos = glm::lerp(node.p_min[axis], node.p_max[axis], splitT);
			vec1 cost = splitEval(axis, splitPos, start, count);
			if (cost < bestCost) {
				bestCost = cost;
				bestSplitPos = splitPos;
				bestSplitAxis = axis;
			}
		}
	}

	axis = bestSplitAxis;
	pos = bestSplitPos;
	cost = bestCost;
}

vec1 Builder::splitEval(const uint8& splitAxis, const vec1& splitPos, const uint& start, const uint& count) const {
	GPU_Bvh boundsLeft = GPU_Bvh();
	GPU_Bvh boundsRight;
	uint numOnLeft = 0;
	uint numOnRight = 0;

	for (uint i = start; i < start + count; i++) {
		Bvh_Particle tri = bvh_particles[i];
		if (tri.center[splitAxis] < splitPos) {
			boundsLeft.growToInclude(tri.p_min, tri.p_max);
			numOnLeft++;
		}
		else {
			boundsRight.growToInclude(tri.p_min, tri.p_max);
			numOnRight++;
		}
	}

	vec1 costA = nodeCost(boundsLeft.getSize(), numOnLeft);
	vec1 costB = nodeCost(boundsRight.getSize(), numOnRight);
	return costA + costB;
}

vec1 Builder::nodeCost(const vec3& size, const uint& numTriangles) {
	const vec1 halfArea = size.x * size.y + size.x * size.z + size.y * size.z;
	return halfArea * numTriangles;
}

Bvh_Particle::Bvh_Particle(const vec3& p_min, const vec3& p_max, const vec3& center, const uint64& index, const CPU_Particle& part) :
	center(center),
	p_min(p_min),
	p_max(p_max),
	index(index),
	part(part)
{}