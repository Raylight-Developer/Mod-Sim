#include "Bvh.hpp"

#include "Kernel.hpp"

#define ITEM_COUNT 4

CPU_Bvh::CPU_Bvh():
	p_min(vec3(MAX_VEC1)),
	p_max(vec3(MIN_VEC1)),
	parent(nullptr),
	item_count(0),
	discard(true)
{}

CPU_Bvh::CPU_Bvh(const vec3& pmin, const vec3& pmax) :
	p_min(pmin),
	p_max(pmax),
	parent(nullptr),
	item_count(0)
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
	start(0),
	end(0)
{}

void CPU_Bvh::growToInclude(const CPU_Probe* probe, const vec1& radius) {
	growToInclude(d_to_f(probe->transformed_position) - radius, d_to_f(probe->transformed_position) + radius);
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

bool CPU_Bvh::contains(const CPU_Probe* probe) {
	const vec3 pos = probe->transformed_position;
	return (pos.x >= p_min.x && pos.x <= p_max.x) &&
		(pos.y >= p_min.y && pos.y <= p_max.y) &&
		(pos.z >= p_min.z && pos.z <= p_max.z);
}

void CPU_Bvh::growToInclude(const CPU_Particle* particle, const vec1& radius) {
	growToInclude(d_to_f(particle->transformed_position) - radius, d_to_f(particle->transformed_position) + radius);
}

bool CPU_Bvh::contains(const CPU_Particle* particle) {
	const vec3 pos = particle->transformed_position;
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


Builder::Builder(const vector<CPU_Probe*>& probes, const vec1& probe_radius, const uint& max_depth) :
	source(probes),
	particle_radius(probe_radius * 1.25f)
{
	if (probe_radius > 0.0f) {
		for (CPU_Probe* probe : source) {
			root_node.growToInclude(probe, probe_radius);
		}
		root_node.discard = false;
		depth_cutoff = max_depth;

		splitBvh(&root_node, 0);

		convertBvh(&root_node);
	}
	else {
		for (CPU_Probe* probe : source) {
			root_node.growToInclude(probe, d_to_f(probe->smoothing_radius));
		}
		root_node.discard = false;
		depth_cutoff = max_depth;

		splitBvh(&root_node, 0);
		growBvhSPH(&root_node);

		convertBvhSPH(&root_node);
	}
}

void Builder::splitBvh(CPU_Bvh* node, const uint& depth) {
	node->split();

	for (auto it = node->children.begin(); it != node->children.end(); ) {
		CPU_Bvh& child = *it;

#ifdef OPENMP
		#pragma omp parallel for
		int i = 0;
		int i_size = ul_to_i(source.size());
		for (i = 0; i < i_size; i++) {
			if (child.contains(source[i])) {
				#pragma omp critical
				{
					child.item_count++;
					if (child.item_count > ITEM_COUNT) {
						break;
					}
				}
			}
		}
#else
		for (const CPU_Probe* probe : source) {
			if (child.contains(probe)) {
				child.discard = false;
				child.item_count++;
				if (child.item_count > ITEM_COUNT) {
					break;
				}
			}
		}
#endif
		if (child.discard == false or child.item_count > 0) {
			if (depth < depth_cutoff and child.item_count > ITEM_COUNT) {
				splitBvh(&child, depth + 1);
			}
			else {
#ifdef OPENMP
				#pragma omp parallel for
				int i = 0;
				int i_size = ul_to_i(particles.size());
				for (i = 0; i < i_size; i++) {
					if (child.contains(particles[i])) {
						#pragma omp critical
						{
							child.particles.push_back(particles[i]);
						}
					}
				}
#else
				for (CPU_Probe* probe : source) {
					if (child.contains(probe)) {
						child.probes.push_back(probe);
					}
				}
#endif
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

	if (node->probes.empty()) {
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
		bvh.start =  ul_to_u(probes.size());
		bvh.end =  ul_to_u(probes.size() + node->probes.size());
		for (const auto& probe : node->probes) {
			probes.push_back(probe);
		}
	}

	nodes[index] = bvh;
	if (node == &root_node) {
		gpu_root_node = bvh;
	}
	return index;
}

void Builder::growBvhSPH(CPU_Bvh* node) {
	if (!node->probes.empty()) {
		dvec1 radius = 0.0f;
		for (CPU_Probe* probe : node->probes) {
			radius = max(radius, probe->smoothing_radius);
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

	if (node->probes.empty()) {
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
		bvh.start =  ul_to_u(probes.size());
		bvh.end =  ul_to_u(probes.size() + node->probes.size());
		for (CPU_Probe* probe : node->probes) {
			probes.push_back(probe);
		}
	}

	nodes[index] = bvh;
	if (node == &root_node) {
		gpu_root_node = bvh;
	}
	return index;
}

Bvh_Particle::Bvh_Particle(const vec3& p_min, const vec3& p_max, const vec3& center, const uint64& index, const CPU_Probe& part) :
	center(center),
	p_min(p_min),
	p_max(p_max),
	index(index),
	part(part)
{}

bool CPU_Bvh::operator==(const CPU_Bvh & other) const {
	return p_min == other.p_min && p_max == other.p_max && discard == other.discard;
}

Particle_Builder::Particle_Builder(const vector<CPU_Particle*>& particles, const vec1& particle_radius, const uint& max_depth) :
	source(particles),
	particle_radius(particle_radius * 1.25f)
{
	for (CPU_Particle* particle : source) {
		root_node.growToInclude(particle, particle_radius);
	}
	root_node.discard = false;
	depth_cutoff = max_depth;

	splitBvh(&root_node, 0);

	convertBvh(&root_node);
}

void Particle_Builder::splitBvh(CPU_Bvh* node, const uint& depth) {
	node->split();

	for (auto it = node->children.begin(); it != node->children.end(); ) {
		CPU_Bvh& child = *it;

#ifdef OPENMP
		#pragma omp parallel for
		int i = 0;
		int i_size = ul_to_i(source.size());
		for (i = 0; i < i_size; i++) {
			if (child.contains(source[i])) {
				#pragma omp critical
				{
					child.item_count++;
					if (child.item_count > ITEM_COUNT) {
						break;
					}
				}
			}
		}
#else
		for (const CPU_Particle* particle : source) {
			if (child.contains(particle)) {
				child.discard = false;
				child.item_count++;
				if (child.item_count > ITEM_COUNT) {
					break;
				}
			}
		}
#endif
		if (child.discard == false or child.item_count > 0) {
			if (depth < depth_cutoff and child.item_count > ITEM_COUNT) {
				splitBvh(&child, depth + 1);
			}
			else {
#ifdef OPENMP
				#pragma omp parallel for
				int i = 0;
				int i_size = ul_to_i(source.size());
				for (i = 0; i < i_size; i++) {
					if (child.contains(source[i])) {
						#pragma omp critical
						{
							child.source.push_back(source[i]);
						}
					}
				}
#else
				for (CPU_Particle* particle : source) {
					if (child.contains(particle)) {
						child.particles.push_back(particle);
					}
				}
#endif
			}
			++it;
		}
		else {
			it = node->children.erase(it);
		}
	}
}

uint Particle_Builder::convertBvh(CPU_Bvh* node) {
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
		bvh.start =  ul_to_u(particles.size());
		bvh.end =  ul_to_u(particles.size() + node->particles.size());
		for (CPU_Particle* particle : node->particles) {
			particles.push_back(particle);
		}
	}

	nodes[index] = bvh;
	if (node == &root_node) {
		gpu_root_node = bvh;
	}
	return index;
}