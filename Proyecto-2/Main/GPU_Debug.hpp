#pragma once

#include "Shared.hpp"

#include "Particle.hpp"
#include "Bvh.hpp"

struct Ray {
	vec3  origin;
	vec3  direction;
};

struct GPU_Debug {
	vector<CPU_Particle> particles;
	vector<GPU_Bvh> bvh_nodes;
	GPU_Bvh root_node;
	Transform camera_transform;
	vec1 sphere_display_radius;

	GPU_Debug(const vector<CPU_Particle>& particles, const vector<GPU_Bvh>& bvh_nodes, const GPU_Bvh& root_node, const vec1& sphere_display_radius);

	Ray f_cameraRay(const vec2& uv);
	bool f_rayBvhIntersection(const Ray& ray, const GPU_Bvh& box);
	bool f_raySphereIntersection(const Ray& ray, const vec3& sphere, vec1& t);
	int f_visitBvh(const Ray& ray, const GPU_Bvh& node, uint& bvh_depth, vec1& result_raylength);
};