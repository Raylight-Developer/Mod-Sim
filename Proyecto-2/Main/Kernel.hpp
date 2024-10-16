#pragma once

#include "Shared.hpp"

struct CPU_Particle {
	dvec1 mass;
	dvec3 pos;
	dvec3 velocity;
	dvec3 acceleration;
};

struct alignas(16) GPU_Particle {
	vec4 pos;
	vec4 color;

	GPU_Particle(const CPU_Particle& particle);
};

struct CPU_Cell {
	vec3 velocity;
	vec3 acceleration;
	vec1 pressure;
	vec1 density;
};

struct GPU_Cell {
	vec3 velocity;
	vec1 pressure;
	vec3 acceleration;
	vec1 density;

	GPU_Cell();
	GPU_Cell(const CPU_Cell& cell);
};

using Grid = vector<vector<vector<CPU_Cell>>>;
dvec3 velocityToColor(const dvec3& velocity);
dvec1 randD();
vec1  randF();

// Particles
void  initialize(vector<CPU_Particle>& points);
void  simulate(vector<CPU_Particle>& points, const dvec1& delta_time);
dvec3 computeVortexForce(const dvec3& pos, const dvec3& vortexCenter,  const dvec1& spinstrength, const dvec1& centrifugalStrength);
//Grid
void initialize(Grid& grid, const ulvec3& size);
void simulate  (Grid& grid, const ulvec3& size, const dvec1& delta_time);
void advection (Grid& grid, const ulvec3& size, const dvec1& delta_time);
void diffusion (Grid& grid, const ulvec3& size, const dvec1& delta_time);
void projection(Grid& grid, const ulvec3& size, const dvec1& delta_time);

void forceSolve(Grid& grid, const ulvec3& size, const dvec1& delta_time);
void pressureSolve(Grid& grid, const ulvec3& size, const dvec1& delta_time);

vec3 vorticitySolve(const Grid& grid, const ulvec3& pos, const ulvec3& size);
vec3 computePressureGradient(const Grid& grid, uint64 x, uint64 y, uint64 z, const uvec3& size);

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

	dmat4 getMatrix() const;
};