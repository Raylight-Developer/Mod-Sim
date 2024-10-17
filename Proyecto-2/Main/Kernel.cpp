#include "Kernel.hpp"

#define AIR_DENSITY       1.225      // kg/m^3 (at sea level)
#define GAS_CONSTANT      287.05     // J/(kg·K) for dry air
#define EARTH_ROTATION    7.2921e-5  // radians per second
#define SPECIFIC_HEAT_AIR 1005.0     // J/(kg·K)
#define STEFAN_BOLTZMANN  5.67e-8    // W/m²·K⁴
#define SEA_SURFACE_TEMP  28.5       // C
#define LATITUDE          15.0

GPU_Particle::GPU_Particle(const CPU_Particle& particle) :
	pos(d_to_f(particle.pos), 1.0f),
	velocity(d_to_f(particle.velocity), 1.0f)
{}

GPU_Cell::GPU_Cell() {
	density = 0.0f;
	humidity = 0.0f;
	pressure = 0.0f;
	temperature = 0.0f;
}

GPU_Cell::GPU_Cell(const CPU_Cell& cell) {
	density     = cell.density;
	humidity    = cell.humidity;
	pressure    = cell.pressure;
	temperature = cell.temperature;
}

void initialize(vector<CPU_Particle>& points) {
	dvec1 radius = 0.5;

	for (uint64 i = 0; i < points.size(); i++) {
		CPU_Particle& particle = points[i];
		const dvec1 angle = i * (glm::pi<dvec1>() * (3.0 - sqrt(5.0)));
		const dvec1 r = radius * sqrt(i / (dvec1)(points.size() - 1));

		const dvec1 x = r * cos(angle);
		const dvec1 y = r * sin(angle);

		particle.mass = randD() * 0.5 + 0.1;
		particle.humidity = randD() * 0.5 + 0.1;
		particle.pressure = randD() * 0.5 + 0.1;
		particle.temperature = 15.0;

		particle.pos = dvec3(x, 0, y);
		particle.velocity = dvec3(0.0);
		particle.acceleration = dvec3(0.0);
	}
}

void simulate(vector<CPU_Particle>& points, const dvec1& delta_time) {
	for (CPU_Particle& particle: points) {
		computeThermodynamics(particle);
		updateVelocity(particle, points, delta_time);
		updatePosition(particle, delta_time);
	}
}

dvec3 computeNavierStokes(const CPU_Particle& particle, const vector<CPU_Particle>& neighbors) {
	dvec3 pressure_gradient(0.0);
	dvec3 velocity_diffusion(0.0);

	for (const auto& neighbor : neighbors) {
		dvec3 distance = particle.pos - neighbor.pos;
		dvec1 dist_squared = length2(distance);

		if (dist_squared > 0.0) {
			pressure_gradient += (neighbor.pressure - particle.pressure) / dist_squared * normalize(distance);
			velocity_diffusion += (neighbor.velocity - particle.velocity) / dist_squared;
		}
	}

	return -pressure_gradient + velocity_diffusion;
}

dvec3 computeCoriolisEffect(const CPU_Particle& particle) {
	const dvec1 coriolisParameter = 2.0 * EARTH_ROTATION * sin(LATITUDE);
	return coriolisParameter * cross(dvec3(0.0, 1.0, 0.0), particle.velocity);
}

dvec1 computeThermodynamics(CPU_Particle& particle) {
	dvec1 radiation_loss = STEFAN_BOLTZMANN * pow(particle.temperature, 4);
	dvec1 heat_gain = SEA_SURFACE_TEMP * particle.humidity * SPECIFIC_HEAT_AIR * (SEA_SURFACE_TEMP - particle.temperature);

	particle.temperature += (heat_gain - radiation_loss) / (particle.mass * SPECIFIC_HEAT_AIR);
	return particle.temperature;
}

void updateVelocity(CPU_Particle& particle, const vector<CPU_Particle>& neighbors, const dvec1& delta_time) {
	dvec3 navier_stokes_force = computeNavierStokes(particle, neighbors);
	dvec3 coriolis_force = computeCoriolisEffect(particle);

	particle.acceleration = (navier_stokes_force + coriolis_force) / particle.mass;
	particle.velocity += particle.acceleration * delta_time;
}

void updatePosition(CPU_Particle& particle, const dvec1& delta_time) {
	particle.pos += particle.velocity * delta_time;
}






void initialize(Grid& grid, const ulvec3& size) {
	dvec3 center = dvec3(size) / 2.0;
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
				cell.density *= randD() * 0.5 + 0.5;
				cell.pressure = randD() * 0.5 + 0.5;
			}
		}
	}
}

void simulate(Grid& grid, const ulvec3& size, const dvec1& delta_time) {
	for (uint64 x = 0; x < size.x; ++x) {
		for (uint64 y = 0; y < size.y; ++y) {
			for (uint64 z = 0; z < size.z; ++z) {
				CPU_Cell& cell = grid[x][y][z];
			}
		}
	}
}

void forceSolve(Grid& grid, const ulvec3& size, const dvec1& delta_time) {
	for (uint64 x = 0; x < size.x; ++x) {
		for (uint64 y = 0; y < size.y; ++y) {
			for (uint64 z = 0; z < size.z; ++z) {
				ulvec3 pos = ulvec3(x, y, z);
				CPU_Cell& cell = grid[x][y][z];
			}
		}
	}
}

dvec3 computePressureGradient(const Grid& grid, const uint64& x, const uint64& y, const uint64& z, const uvec3& size) {
	dvec3 gradient(0.0);

	// Check boundaries and calculate differences between neighboring cells
	if (x > 0) gradient.x += grid[x - 1][y][z].pressure - grid[x][y][z].pressure;
	if (x < size.x - 1) gradient.x += grid[x + 1][y][z].pressure - grid[x][y][z].pressure;

	if (y > 0) gradient.y += grid[x][y - 1][z].pressure - grid[x][y][z].pressure;
	if (y < size.y - 1) gradient.y += grid[x][y + 1][z].pressure - grid[x][y][z].pressure;

	if (z > 0) gradient.z += grid[x][y][z - 1].pressure - grid[x][y][z].pressure;
	if (z < size.z - 1) gradient.z += grid[x][y][z + 1].pressure - grid[x][y][z].pressure;

	return gradient;
}

Transform::Transform(const dvec3& position, const dvec3& rotation, const dvec3& scale, const Rotation_Type& type) :
	rotation_type(type),
	position(position),
	euler_rotation(rotation),
	scale(scale)
{
	quat_rotation = dquat(1.0, 0.0, 0.0, 0.0);
	axis_rotation = dvec3(0.0);

	x_vec = dvec3(1.0, 0.0, 0.0);
	y_vec = dvec3(0.0, 1.0, 0.0);
	z_vec = dvec3(0.0, 0.0, 1.0);
}

Transform::Transform(const dvec3& position, const dvec3& axis, const dvec3& rotation, const dvec3& scale, const Rotation_Type& type) :
	rotation_type(type),
	position(position),
	euler_rotation(rotation),
	scale(scale),
	axis_rotation(axis)
{
	quat_rotation = dquat(1.0, 0.0, 0.0, 0.0);

	x_vec = dvec3(1.0, 0.0, 0.0);
	y_vec = dvec3(0.0, 1.0, 0.0);
	z_vec = dvec3(0.0, 0.0, 1.0);
}

Transform::Transform(const dvec3& position, const dquat& rotation, const dvec3& scale, const Rotation_Type& type) :
	rotation_type(type),
	position(position),
	quat_rotation(rotation),
	scale(scale)
{
	euler_rotation = dvec3(0.0);
	axis_rotation = dvec3(0.0);

	x_vec = dvec3(1.0, 0.0, 0.0);
	y_vec = dvec3(0.0, 1.0, 0.0);
	z_vec = dvec3(0.0, 0.0, 1.0);
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
	f_computeVectors();
	position += value.x * x_vec;
	position += value.y * y_vec;
	position += value.z * z_vec;
}

void Transform::rotate(const dvec3& value) {
	switch (rotation_type) {
		case Rotation_Type::QUATERNION: {
			const dquat pitch = glm::angleAxis(glm::radians(value.x), dvec3(1, 0, 0));
			const dquat yaw   = glm::angleAxis(glm::radians(value.y), dvec3(0, 1, 0));
			const dquat roll  = glm::angleAxis(glm::radians(value.z), dvec3(0, 0, 1));

			quat_rotation = yaw * pitch * roll * quat_rotation;
			quat_rotation = glm::normalize(quat_rotation);
			break;
		}
		case Rotation_Type::XYZ: {
			euler_rotation += value;

			if (euler_rotation.x > 89.0)  euler_rotation.x = 89.0;
			if (euler_rotation.x < -89.0) euler_rotation.x = -89.0;
		}
	}
}

void Transform::orbit(const dvec3& pivot, const dvec2& py_rotation) {
	switch (rotation_type) {
		case Rotation_Type::QUATERNION: {
			rotate(glm::vec3(py_rotation.x, py_rotation.y, 0.0));

			const dvec3 forward   = glm::normalize(glm::inverse(quat_rotation) * dvec3(0, 0, -1));
			const dvec3 direction = glm::normalize(position - pivot);
			const dvec1 z_distance  = glm::length(position - pivot);

			position = pivot - forward * z_distance;
			break;
		}
		case Rotation_Type::XYZ: {
			rotate(dvec3(py_rotation.x, py_rotation.y, 0.0));

			const dmat4 matrix = glm::yawPitchRoll(glm::radians(euler_rotation.y), glm::radians(euler_rotation.x), glm::radians(euler_rotation.z));
			const dvec3 z_vector = -matrix[2];

			const dvec1 z_distance = glm::length(pivot - position);
			const dvec3 camera_position = position - z_vector * z_distance;

			position = pivot - z_vector * z_distance;
			break;
		}
	}
}

void Transform::f_computeVectors() {
	dmat4 rotation_matrix;
	switch (rotation_type) {
		case Rotation_Type::QUATERNION: {
			rotation_matrix = glm::mat4_cast(quat_rotation);
			break;
		}
		case Rotation_Type::XYZ: {
			rotation_matrix = glm::yawPitchRoll(glm::radians(euler_rotation.y), glm::radians(euler_rotation.x), glm::radians(euler_rotation.z));
			break;
		}
	}
	x_vec = rotation_matrix[0];
	y_vec = rotation_matrix[1];
	z_vec = rotation_matrix[2];
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