#pragma once

#include "Shared.hpp"

struct CPU_Particle {
	dvec1 mass;
	dvec1 humidity;
	dvec1 pressure;
	dvec1 temperature;

	dvec3 pos;
	dvec3 velocity;
	dvec3 acceleration;
};

struct alignas(16) GPU_Particle {
	vec4 pos;
	vec4 velocity;

	GPU_Particle();
	GPU_Particle(const CPU_Particle& particle);
};

struct CPU_Cell {
	dvec1 density;
	dvec1 humidity;
	dvec1 pressure;
	dvec1 temperature;
};

struct GPU_Cell {
	vec1 density;
	vec1 humidity;
	vec1 pressure;
	vec1 temperature;

	GPU_Cell();
	GPU_Cell(const CPU_Cell& cell);
};

using Grid = vector<vector<vector<CPU_Cell>>>;

// Particles
void  initialize(vector<CPU_Particle>& points);
void  simulate(vector<CPU_Particle>& points, const dvec1& delta_time);
dvec3 computeNavierStokes(const CPU_Particle& particle, const vector<CPU_Particle>& neighbors);
dvec3 computeCoriolisEffect(const CPU_Particle& particle);
dvec1 computeThermodynamics(CPU_Particle& particle);
void updateVelocity(CPU_Particle& particle, const vector<CPU_Particle>& neighbors, const dvec1& delta_time);
void updatePosition(CPU_Particle& particle, const dvec1& delta_time);

//Grid
void initialize(Grid& grid, const ulvec3& size);
void simulate  (Grid& grid, const ulvec3& size, const dvec1& delta_time);
dvec3 computePressureGradient(const Grid& grid, const uint64& x, const uint64& y, const uint64& z, const uvec3& size);

void advection (Grid& grid, const ulvec3& size, const dvec1& delta_time);
void diffusion (Grid& grid, const ulvec3& size, const dvec1& delta_time);
void projection(Grid& grid, const ulvec3& size, const dvec1& delta_time);

//Util
enum struct Rotation_Type {
	QUATERNION,
	AXIS,
	XYZ,
	XZY,
	YXZ,
	YZX,
	ZXY,
	ZYX
};

struct Transform {
	Rotation_Type rotation_type;
	dvec3 euler_rotation;
	dvec3 axis_rotation;
	dquat quat_rotation;
	dvec3 position;
	dvec3 scale;

	dvec3 x_vec;
	dvec3 y_vec;
	dvec3 z_vec;

	Transform(const dvec3& position = dvec3(0.0), const dvec3& rotation = dvec3(0.0), const dvec3& scale = dvec3(1.0), const Rotation_Type& type = Rotation_Type::XYZ);
	Transform(const dvec3& position, const dvec3& axis, const dvec3& rotation, const dvec3& scale, const Rotation_Type& type = Rotation_Type::AXIS);
	Transform(const dvec3& position, const dquat& rotation, const dvec3& scale, const Rotation_Type& type = Rotation_Type::QUATERNION);

	Transform operator+(const Transform& other) const;
	Transform operator-(const Transform& other) const;
	Transform operator*(const Transform& other) const;
	Transform operator/(const Transform& other) const;

	Transform operator*(const dvec1& other) const;

	void moveLocal(const dvec3& value);
	void rotate(const dvec3& value);
	void orbit(const dvec3& pivot, const dvec2& py_rotation);
	void f_computeVectors();

	dmat4 getMatrix() const;
};