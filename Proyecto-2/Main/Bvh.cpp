#include "Bvh.hpp"

#include "Kernel.hpp"

CPU_Bvh::CPU_Bvh():
	p_min(vec3(MAX_VEC1)),
	p_max(vec3(MIN_VEC1))
{}

CPU_Bvh::CPU_Bvh(const vec3& pmin, const vec3& pmax) :
	p_min(pmin),
	p_max(pmax)
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
	particle_pointer(0),
	particle_count(0)
{}

void CPU_Bvh::growToInclude(const CPU_Particle& particle, const vec1& radius) {
	growToInclude(particle.position - radius, particle.position + radius);
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
	const vec3 pos = particle.position;
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
}


Builder::Builder(const vector<CPU_Particle>& particles, const vec1& particle_radius, const uint& depth) :
	particles(particles),
	particle_radius(particle_radius)
{
	for (uint i = 0; i < len32(particles); i++) {
		const vec3 center = (particles[i].position);
		const vec3 max = particles[i].position + particle_radius;
		const vec3 min = particles[i].position - particle_radius;
		root_node.growToInclude(min, max);
	}
	root_node.discard = false;
	max_depth = depth;

	splitBvh(&root_node, 0);

	this->particles.clear();
	convertBvh(&root_node);
}

void Builder::splitBvh(CPU_Bvh* parent, const uint& depth) {
	parent->split();

	for (auto it = parent->children.begin(); it != parent->children.end(); ) {
		CPU_Bvh& child = *it;
		for (const CPU_Particle& particle : particles) {
			if (child.contains(particle)) {
				child.discard = false;
				break;
			}
		}
		if (child.discard == false) {
			if (depth < max_depth) {
				splitBvh(&child, depth + 1);
			}
			else {
				for (const CPU_Particle& particle : particles) {
					if (child.contains(particle)) {
						child.particles.push_back(particle);
						//child.growToInclude(particle, particle_radius);
					}
				}
				child.p_min += 0.1;
				child.p_max -= 0.1;
			}
			++it;
		}
		else {
			it = parent->children.erase(it);
		}
	}
}

void Builder::convertBvh(CPU_Bvh* node) {
	auto bvh = GPU_Bvh();
	bvh.p_min = node->p_min;
	bvh.p_max = node->p_max;

	uint index = nodes.size();
	nodes.push_back(bvh);

	if (node->particles.empty()) {
		for (uint i = 0; i < node->children.size(); i++) {
			CPU_Bvh& child = node->children[i];
			if (i < 4) {
				bvh.pointers_a[i] = index + i + 1;
			}
			else {
				bvh.pointers_b[i - 4] = index + i + 1;
			}
			convertBvh(&child);
		}
	}
	else {
		bvh.particle_pointer = particles.size();
		bvh.particle_count = node->particles.size();
		for (const auto& particle : node->particles) {
			particles.push_back(particle);
		}
	}

	nodes[index] = bvh;
	if (node == &root_node) {
		gpu_root_node = bvh;
	}
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
