#include "Kernel.hpp"


GPU_Particle::GPU_Particle(const CPU_Particle& particle) :
	pos(vec4(d_to_f(particle.pos), 1.0f)),
	color(vec4(d_to_f(velocityToColor(particle.velocity)), 1.0f))
{}

GPU_Cell::GPU_Cell() {
	velocity = vec3(0.0f);
	acceleration = vec3(0.0f);
	pressure = 0.0f;
	density = 0.0f;
}

GPU_Cell::GPU_Cell(const CPU_Cell& cell) {
	velocity = cell.velocity;
	acceleration = cell.acceleration;
	pressure = cell.pressure;
	density = cell.density;
}

dvec3 velocityToColor(const dvec3& velocity) {
	const dvec1 maxSpeed = 10.0;
	dvec1 speed = glm::length(velocity);

	speed = glm::clamp(speed, 0.0, maxSpeed);
	const dvec1 normalizedSpeed = speed / maxSpeed;
	dvec3 color;

	if (normalizedSpeed <= 0.5) {
		dvec1 t = normalizedSpeed * 2.0;
		color = glm::mix(dvec3(0.0, 0.0, 1.0), dvec3(0.0, 1.0, 0.0), t);
	}
	else {
		dvec1 t = (normalizedSpeed - 0.5) * 2.0;
		color = glm::mix(dvec3(0.0, 1.0, 0.0), dvec3(1.0, 0.0, 0.0), t);
	}

	return color;
}

dvec1 randD() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	uniform_real_distribution<dvec1> dis(0.0, 1.0);
	return dis(gen);
}

void initialize(vector<CPU_Particle>& points) {
	float radius = 0.5;

	uint i = 0;
	for (CPU_Particle& particle: points) {
		//particle.color = vec4(1, 1, 1, 1);
		particle.velocity = dvec3(0.01, 0.01, 0);

		const float angle = i * (glm::pi<float>() * (3.0f - sqrt(5.0f)));
		const float r = radius * sqrt(i / (float)(points.size() - 1));

		const float x = r * cos(angle);
		const float y = r * sin(angle);

		points[i].pos = vec4(x, 0, y, 1);
		i++;
	}
}

void simulate(vector<CPU_Particle>& points, const dvec1& time, const bool& openmp) {
	uint i = 0;
	for (CPU_Particle& particle: points) {
		//particle.pos += particle.velocity * time;
		i++;
	}
}

void initialize(Grid& grid, const ulvec3& size) {
	dvec3 center = dvec3(size.x / 2.0, size.y / 2.0, size.z / 2.0);
	dvec1 max_distance = glm::max(glm::max(size.x, size.y), size.z) / 2.0;

	for (uint64 x = 0; x < size.x; ++x) {
		for (uint64 y = 0; y < size.y; ++y) {
			for (uint64 z = 0; z < size.z; ++z) {
				CPU_Cell& cell = grid[x][y][z];

				dvec3 current_pos = dvec3(x, y, z);
				dvec1 distance = glm::length(current_pos - center);
				if (distance <= max_distance) {
					cell.density = 1.0f - (distance / max_distance);
				} else {
					cell.density = 0.0f;
				}
				cell.density += randD() * 0.5;
				cell.velocity = vec3(randD(), randD(), randD()) * 2.0f - 1.0f;
				cell.acceleration = vec3(0);
			}
		}
	}
}

void simulate(Grid& grid, const ulvec3& size, const dvec1& delta_time) {
	//for (uint64 x = 0; x < size.x; ++x) {
	//	for (uint64 y = 0; y < size.y; ++y) {
	//		for (uint64 z = 0; z < size.z; ++z) {
	//			CPU_Cell& cell = grid[x][y][z];
	//			//cell.velocity += cell.acceleration * static_cast<float>(delta_time);
	//		}
	//	}
	//}
	forceSolve(grid, size, delta_time);
}

void forceSolve(Grid& grid, const ulvec3& size, const dvec1& delta_time) {
	for (uint64 x = 0; x < size.x; ++x) {
		for (uint64 y = 0; y < size.y; ++y) {
			for (uint64 z = 0; z < size.z; ++z) {
				ulvec3 pos = ulvec3(x, y, z);
				CPU_Cell& cell = grid[x][y][z];

				vec3 cell_position = vec3(x, y, z);
				vec3 tornado_center = vec3(size.x / 2.0, size.y / 2.0, size.z / 2.0);
				vec1 vortex_radius = 32.0;
				vec1 tornado_strength = 1.0;
				vec3 to_center = tornado_center - cell_position;
				float distance_to_center = glm::length(to_center);

				// If within the vortex radius, apply tornado forces
				if (distance_to_center < vortex_radius) {
					// Normalize the vector to center
					vec3 direction_to_center = glm::normalize(to_center);

					// Apply inward force (toward the tornado center)
					vec3 inward_force = direction_to_center * tornado_strength * (1.0f - (distance_to_center / vortex_radius));

					// Apply rotational force (perpendicular to the inward force, creating a spiral)
					vec3 rotational_direction = glm::cross(direction_to_center, vec3(0, 1, 0));  // Rotate around the y-axis (upward)
					vec3 rotational_force = rotational_direction * tornado_strength * (1.0f - (distance_to_center / vortex_radius));

					// Sum of forces: inward + rotational
					vec3 total_force = inward_force + rotational_force;

					// Update cell's acceleration based on the total force applied
					cell.acceleration += total_force;

					// Update velocity based on acceleration and delta time (simple integration)
					cell.velocity += cell.acceleration * d_to_f(delta_time);

					// Optionally, reset acceleration after each step if you're accumulating forces
					cell.acceleration = vec3(0.0f);
				}
			}
		}
	}
}

vec3 vorticitySolve(const Grid& grid, const ulvec3& pos, const ulvec3& size) {
	vec3 vorticity(0.0f);

	if (pos.x > 0 && pos.x < size.x - 1 && pos.y > 0 && pos.y < size.y - 1 && pos.z > 0 && pos.z < size.z - 1) {
		vec3 vel_right = grid[pos.x + 1][pos.y][pos.z].velocity;
		vec3 vel_left = grid[pos.x - 1][pos.y][pos.z].velocity;
		vec3 vel_up = grid[pos.x][pos.y + 1][pos.z].velocity;
		vec3 vel_down = grid[pos.x][pos.y - 1][pos.z].velocity;
		vec3 vel_front = grid[pos.x][pos.y][pos.z + 1].velocity;
		vec3 vel_back = grid[pos.x][pos.y][pos.z - 1].velocity;

		vorticity.x = (vel_front.y - vel_back.y) - (vel_up.z - vel_down.z);
		vorticity.y = (vel_right.z - vel_left.z) - (vel_front.x - vel_back.x);
		vorticity.z = (vel_up.x - vel_down.x) - (vel_right.y - vel_left.y);
	}

	return vorticity;
}

Transform::Transform(const dvec3& position, const dvec3& rotation, const dvec3& scale, const Rotation_Type& type) :
	rotation_type(type),
	position(position),
	euler_rotation(rotation),
	scale(scale)
{
	quat_rotation = dquat(1.0, 0.0, 0.0, 0.0);
	axis_rotation = dvec3(0.0);
}

Transform::Transform(const dvec3& position, const dvec3& axis, const dvec3& rotation, const dvec3& scale, const Rotation_Type& type) :
	rotation_type(type),
	position(position),
	euler_rotation(rotation),
	scale(scale),
	axis_rotation(axis)
{
	quat_rotation = dquat(1.0, 0.0, 0.0, 0.0);
}

Transform::Transform(const dvec3& position, const dquat& rotation, const dvec3& scale, const Rotation_Type& type) :
	rotation_type(type),
	position(position),
	quat_rotation(rotation),
	scale(scale)
{
	euler_rotation = dvec3(0.0);
	axis_rotation = dvec3(0.0);
}
// TODO account for different rotation modes
Transform Transform::operator+(const Transform& other) const {
	Transform result;
	result.position       = position       + other.position;
	result.euler_rotation = euler_rotation + other.euler_rotation;
	result.axis_rotation  = axis_rotation  + other.axis_rotation;
	//result.quat_rotation  = quat_rotation  + other.quat_rotation;
	result.scale          = scale          + other.scale;
	return result;
}
Transform Transform::operator-(const Transform& other) const {
	Transform result;
	result.position       = position       - other.position;
	result.euler_rotation = euler_rotation - other.euler_rotation;
	result.axis_rotation  = axis_rotation  - other.axis_rotation;
	//result.quat_rotation  = quat_rotation  - other.quat_rotation;
	result.scale          = scale          - other.scale;
	return result;
}
Transform Transform::operator*(const Transform& other) const {
	Transform result;
	result.position       = position       * other.position;
	result.euler_rotation = euler_rotation * other.euler_rotation;
	result.axis_rotation  = axis_rotation  * other.axis_rotation;
	//result.quat_rotation  = quat_rotation  * other.quat_rotation;
	result.scale          = scale          * other.scale;
	return result;
}
Transform Transform::operator/(const Transform& other) const {
	Transform result;
	result.position       = position       / other.position;
	result.euler_rotation = euler_rotation / other.euler_rotation;
	result.axis_rotation  = axis_rotation  / other.axis_rotation;
	//result.quat_rotation  = quat_rotation  / other.quat_rotation;
	result.scale          = scale          / other.scale;
	return result;
}

Transform Transform::operator*(const dvec1& other) const {
	Transform result;
	result.position       = position       * other;
	result.euler_rotation = euler_rotation * other;
	result.axis_rotation  = axis_rotation  * other;
	//result.quat_rotation  = quat_rotation  * other;
	result.scale          = scale          * other;
	return result;
}

void Transform::moveLocal(const dvec3& value) {
	const dmat4 matrix = glm::yawPitchRoll(euler_rotation.y * DEG_RAD, euler_rotation.x * DEG_RAD, euler_rotation.z * DEG_RAD);
	const dvec3 x_vector = matrix[0];
	const dvec3 y_vector = matrix[1];
	const dvec3 z_vector = matrix[2];
	position += value.x * x_vector;
	position += value.y * y_vector;
	position += value.z * z_vector;
}

void Transform::rotate(const dvec3& value) {
	euler_rotation += value;

	if (euler_rotation.x > 89.0)  euler_rotation.x = 89.0;
	if (euler_rotation.x < -89.0) euler_rotation.x = -89.0;
}

dmat4 Transform::getMatrix() const {
	const dmat4 translation_matrix = glm::translate(dmat4(1.0), position);
	const dmat4 scale_matrix = glm::scale(dmat4(1.0), scale);
	dmat4 rotation_matrix = dmat4(1.0);

	switch (rotation_type) {
		case Rotation_Type::QUATERNION: {
			rotation_matrix = glm::toMat4(quat_rotation);
			break;
		}
		case Rotation_Type::XYZ: {
			const dmat4 rotationX = glm::rotate(dmat4(1.0), euler_rotation.x * DEG_RAD, dvec3(1.0, 0.0, 0.0));
			const dmat4 rotationY = glm::rotate(dmat4(1.0), euler_rotation.y * DEG_RAD, dvec3(0.0, 1.0, 0.0));
			const dmat4 rotationZ = glm::rotate(dmat4(1.0), euler_rotation.z * DEG_RAD, dvec3(0.0, 0.0, 1.0));
			rotation_matrix =  rotationZ * rotationY * rotationX;
			break;
		}
	}
	return translation_matrix * rotation_matrix * scale_matrix;
}