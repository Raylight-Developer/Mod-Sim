#include "Bvh.hpp"

#include "Kernel.hpp"

CPU_Bvh::CPU_Bvh():
	p_min(vec3(MAX_VEC1)),
	p_max(vec3(MIN_VEC1)),
	parent(nullptr),
	particle_count(0),
	discard(true)
{}

CPU_Bvh::CPU_Bvh(const vec3& pmin, const vec3& pmax) :
	p_min(pmin),
	p_max(pmax),
	parent(nullptr),
	particle_count(0)
{
	if (p_min.x > p_max.x) {
		auto temp = p_min.x;
		p_min.x = p_max.x;
		p_max.x = temp;
	}
	if (p_min.y > p_max.y) {
		auto temp = p_min.y;
		p_min.y = p_max.y;
		p_max.y = temp;
	}
	if (p_min.z > p_max.z) {
		auto temp = p_min.z;
		p_min.z = p_max.z;
		p_max.z = temp;
	}
	discard = true;
}

GPU_Bvh::GPU_Bvh() :
	p_min(vec3(MAX_VEC1)),
	p_max(vec3(MIN_VEC1)),
	pointers_a(ivec4(-1)),
	pointers_b(ivec4(-1)),
	particle_start(0),
	particle_end(0)
{}

void CPU_Bvh::growToInclude(const CPU_Particle& particle, const vec1& radius) {
	growToInclude(d_to_f(particle.transformed_position) - radius, d_to_f(particle.transformed_position) + radius);
}

void CPU_Bvh::growToInclude(const vec3& min, const vec3& max) {
	p_min.x = min.x < p_min.x ? min.x : p_min.x;
	p_min.y = min.y < p_min.y ? min.y : p_min.y;
	p_min.z = min.z < p_min.z ? min.z : p_min.z;
	p_max.x = max.x > p_max.x ? max.x : p_max.x;
	p_max.y = max.y > p_max.y ? max.y : p_max.y;
	p_max.z = max.z > p_max.z ? max.z : p_max.z;
}

vec3 CPU_Bvh::getSize() const {
	return p_max - p_min;
}

vec3 CPU_Bvh::getCenter() const {
	return (p_min + p_max) / 2.0f;
}

bool CPU_Bvh::contains(const CPU_Particle& particle) {
	const vec3 pos = particle.transformed_position;
	return (pos.x >= p_min.x && pos.x <= p_max.x) &&
		(pos.y >= p_min.y && pos.y <= p_max.y) &&
		(pos.z >= p_min.z && pos.z <= p_max.z);
}

void CPU_Bvh::split() {
	children.clear();

	vec3 mid = getCenter();

	children.push_back(CPU_Bvh(vec3(p_min.x, p_min.y, p_min.z), vec3(mid.x, mid.y, mid.z)));
	children.push_back(CPU_Bvh(vec3(mid.x, p_min.y, p_min.z), vec3(p_max.x, mid.y, mid.z)));
	children.push_back(CPU_Bvh(vec3(p_min.x, mid.y, p_min.z), vec3(mid.x, p_max.y, mid.z)));
	children.push_back(CPU_Bvh(vec3(mid.x, mid.y, p_min.z), vec3(p_max.x, p_max.y, mid.z)));
	children.push_back(CPU_Bvh(vec3(p_min.x, p_min.y, mid.z), vec3(mid.x, mid.y, p_max.z)));
	children.push_back(CPU_Bvh(vec3(mid.x, p_min.y, mid.z), vec3(p_max.x, mid.y, p_max.z)));
	children.push_back(CPU_Bvh(vec3(p_min.x, mid.y, mid.z), vec3(mid.x, p_max.y, p_max.z)));
	children.push_back(CPU_Bvh(vec3(mid.x, mid.y, mid.z), vec3(p_max.x, p_max.y, p_max.z)));

	for (CPU_Bvh& child : children) {
		child.parent = this;
	}
}


Builder::Builder(const vector<CPU_Particle>& particles, const vec1& particle_radius, const uint& max_depth) :
	particles(particles),
	particle_radius(particle_radius * 1.25f)
{
	if (particle_radius > 0.0f) {
		for (uint i = 0; i < particles.size(); i++) {
			const vec3 center = (particles[i].transformed_position);
			const vec3 max = d_to_f(particles[i].transformed_position) + particle_radius;
			const vec3 min = d_to_f(particles[i].transformed_position) - particle_radius;
			root_node.growToInclude(min, max);
		}
		root_node.discard = false;
		depth_cutoff = max_depth;

		splitBvh(&root_node, 0);

		this->particles.clear();
		convertBvh(&root_node);
	}
	else {
		for (uint i = 0; i < particles.size(); i++) {
			const vec3 center = (particles[i].transformed_position);
			const vec3 max = d_to_f(particles[i].transformed_position + particles[i].smoothing_radius);
			const vec3 min = d_to_f(particles[i].transformed_position - particles[i].smoothing_radius);
			root_node.growToInclude(min, max);
		}
		root_node.discard = false;
		depth_cutoff = max_depth;

		splitBvhSPH(&root_node, 0);
		growBvhSPH(&root_node);

		this->particles.clear();
		convertBvhSPH(&root_node);
	}
}

void Builder::splitBvh(CPU_Bvh* node, const uint& depth) {
	node->split();

	for (auto it = node->children.begin(); it != node->children.end(); ) {
		CPU_Bvh& child = *it;
		for (const CPU_Particle& particle : particles) {
			if (child.contains(particle)) {
				child.discard = false;
				child.particle_count++;
				if (child.particle_count > 16) {
					break;
				}
			}
		}
		if (child.discard == false) {
			if (depth < depth_cutoff and child.particle_count > 16) {
				splitBvh(&child, depth + 1);
			}
			else {
				for (const CPU_Particle& particle : particles) {
					if (child.contains(particle)) {
						child.particles.push_back(particle);
					}
				}
			}
			++it;
		}
		else {
			it = node->children.erase(it);
		}
	}
}

uint Builder::convertBvh(CPU_Bvh* node) {
	auto bvh = GPU_Bvh();
	bvh.p_min = node->p_min - particle_radius;
	bvh.p_max = node->p_max + particle_radius;

	uint index = ul_to_u(nodes.size());
	nodes.push_back(bvh);

	if (node->particles.empty()) {
		for (uint i = 0; i < node->children.size(); i++) {
			CPU_Bvh& child = node->children[i];
			if (i < 4) {
				bvh.pointers_a[i] = convertBvh(&child);
			}
			else {
				bvh.pointers_b[i - 4] = convertBvh(&child);
			}
		}
	}
	else {
		bvh.particle_start =  ul_to_u(particles.size());
		bvh.particle_end =  ul_to_u(particles.size() + node->particles.size());
		for (const auto& particle : node->particles) {
			particles.push_back(particle);
		}
	}

	nodes[index] = bvh;
	if (node == &root_node) {
		gpu_root_node = bvh;
	}
	return index;
}

void Builder::splitBvhSPH(CPU_Bvh* node, const uint& depth) {
	node->split();

	for (auto it = node->children.begin(); it != node->children.end(); ) {
		CPU_Bvh& child = *it;
		for (const CPU_Particle& particle : particles) {
			if (child.contains(particle)) {
				child.discard = false;
				child.particle_count++;
				if (child.particle_count > 16) {
					break;
				}
			}
		}
		if (child.discard == false) {
			if (depth < depth_cutoff and child.particle_count > 16) {
				splitBvhSPH(&child, depth + 1);
			}
			else {
				for (const CPU_Particle& particle : particles) {
					if (child.contains(particle)) {
						child.particles.push_back(particle);
					}
				}
			}
			++it;
		}
		else {
			it = node->children.erase(it);
		}
	}
}

void Builder::growBvhSPH(CPU_Bvh* node) {
	if (!node->particles.empty()) {
		dvec1 radius = 0.0f;
		for (CPU_Particle& particle : node->particles) {
			radius = max(radius, particle.smoothing_radius);
		}
		node->p_min -= d_to_f(radius);
		node->p_max += d_to_f(radius);

		CPU_Bvh* parent = node->parent;
		while (parent) {
			parent->growToInclude(node->p_min, node->p_max);
			parent = parent->parent;
		}
	}
	for (CPU_Bvh& child : node->children) {
		growBvhSPH(&child);
	}
}

uint Builder::convertBvhSPH(CPU_Bvh* node) {
	auto bvh = GPU_Bvh();

	bvh.p_min = node->p_min;
	bvh.p_max = node->p_max;

	uint index = ul_to_u(nodes.size());
	nodes.push_back(bvh);

	if (node->particles.empty()) {
		for (uint i = 0; i < node->children.size(); i++) {
			CPU_Bvh& child = node->children[i];
			if (i < 4) {
				bvh.pointers_a[i] = convertBvhSPH(&child);
			}
			else {
				bvh.pointers_b[i - 4] = convertBvhSPH(&child);
			}
		}
	}
	else {
		bvh.particle_start =  ul_to_u(particles.size());
		bvh.particle_end =  ul_to_u(particles.size() + node->particles.size());
		for (const auto& particle : node->particles) {
			particles.push_back(particle);
		}
	}

	nodes[index] = bvh;
	if (node == &root_node) {
		gpu_root_node = bvh;
	}
	return index;
}

Bvh_Particle::Bvh_Particle(const vec3& p_min, const vec3& p_max, const vec3& center, const uint64& index, const CPU_Particle& part) :
	center(center),
	p_min(p_min),
	p_max(p_max),
	index(index),
	part(part)
{}

bool CPU_Bvh::operator==(const CPU_Bvh & other) const {
	return p_min == other.p_min && p_max == other.p_max && discard == other.discard;
}