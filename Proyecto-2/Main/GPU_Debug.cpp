#include "GPU_Debug.hpp"

GPU_Debug::GPU_Debug(const vector<CPU_Particle>& particles, const vector<GPU_Bvh>& bvh_nodes, const GPU_Bvh& root_node, const vec1& sphere_display_radius) :
	particles(particles),
	bvh_nodes(bvh_nodes),
	root_node(root_node),
	sphere_display_radius(sphere_display_radius)
{
	camera_transform = Transform(dvec3(0, 0, 37.5), dvec3(0));
	camera_transform.orbit(dvec3(0), dvec3(-15, 15, 0));

	uint bvh_depth = 0;
	vec1 t_length = MAX_DIST;
	Ray ray = f_cameraRay(vec2(0.0));
	int i = f_visitBvh(ray, root_node, bvh_depth, t_length);
	cout << i << endl;
}

Ray GPU_Debug::f_cameraRay(const vec2& uv) {
	const mat4 matrix = d_to_f(glm::yawPitchRoll(camera_transform.euler_rotation.y * DEG_RAD, camera_transform.euler_rotation.x * DEG_RAD, camera_transform.euler_rotation.z * DEG_RAD));
	const vec3 y_vector = matrix[1];
	const vec3 z_vector = -matrix[2];

	const vec1 focal_length = 0.05f;
	const vec1 sensor_size  = 0.036f;

	const vec3 projection_center = d_to_f(camera_transform.position) + focal_length * z_vector;
	const vec3 projection_u = normalize(cross(z_vector, y_vector)) * sensor_size;
	const vec3 projection_v = normalize(cross(projection_u, z_vector)) * sensor_size;

	return Ray(d_to_f(camera_transform.position), glm::normalize(projection_center + (projection_u * uv.x) + (projection_v * uv.y) - d_to_f(camera_transform.position)));
}

bool GPU_Debug::f_rayBvhIntersection(const Ray& ray, const GPU_Bvh& box) {
	vec3 f = (box.p_min - ray.origin) * ray.direction;
	vec3 n = (box.p_max - ray.origin) * ray.direction;

	vec3 tmax = max(f, n);
	vec3 tmin = min(f, n);

	vec1 t1 = min(tmax.x, min(tmax.y, tmax.z));
	vec1 t0 = max(tmin.x, max(tmin.y, tmin.z));

	if (t1 >= t0) {
		return true;
	}
	return false;
}

bool GPU_Debug::f_raySphereIntersection(const Ray& ray, const vec3& sphere, vec1& t) {
	vec3 CO = ray.origin - sphere;
	float a = dot(ray.direction, ray.direction);
	float b = 2.0 * dot(ray.direction, CO);
	float c = dot(CO, CO) - sphere_display_radius * sphere_display_radius;
	float delta = b * b - 4.0 * a * c;
	if (delta < 0.0) {
		return false;
	}
	t = (-b - sqrt(delta)) / (2.0 * a);
	return true;
}

int GPU_Debug::f_visitBvh(const Ray& ray, const GPU_Bvh& node, uint& bvh_depth, vec1& result_raylength) {
	Ray bvh_ray = Ray(ray.origin, glm::normalize(1.0f / ray.direction));

	float t_length = MAX_DIST;
	float t_dist = MAX_DIST;

	int closest_particle = -1;
	int stack[64];
	int stack_index = 0;
	stack[stack_index++] = 0;

	while (stack_index > 0) {
		--stack_index;
		bvh_depth++;

		int currentNode = stack[stack_index];
		GPU_Bvh node = bvh_nodes[currentNode];

		if (!f_rayBvhIntersection(bvh_ray, node)) {
			continue;
		}
		if (node.particle_count > 0) { // Leaf
			for (uint i = node.particle_pointer; i < node.particle_count; ++i) {
				if (f_raySphereIntersection(ray, particles[i].position, t_dist)) {
					if (t_dist < t_length) {
						t_length = t_dist;
						result_raylength = t_dist;
						closest_particle = i;
					}
				}
			}
		}
		else {
			for (int i = 0; i < 8; ++i) {
				if (i < 4) {
					int childIndex = node.pointers_a[i];
					if (childIndex >= 0) {
						stack[stack_index++] = childIndex;
					}
				}
				else {
					int childIndex = node.pointers_b[i - 4];
					if (childIndex >= 0) {
						stack[stack_index++] = childIndex;
					}
				}
			}
		}
	}
	return closest_particle;
}