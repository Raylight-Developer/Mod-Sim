#pragma once

#include "Shared.hpp"

struct CPU_Particle {
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
	vec2 velocity;
	vec1 pressure;
	vec1 density;
};

struct GPU_Cell {
	vec2 velocity;
	vec1 pressure;
	vec1 density;

	GPU_Cell();
	GPU_Cell(const CPU_Cell& cell);
};

using Grid = vector<vector<vector<CPU_Cell>>>;

dvec3 velocityToColor(const dvec3& velocity);
void  initialize(vector<CPU_Particle>& points);
void  simulate(vector<CPU_Particle>& points, const dvec1& time, const bool& openmp);

void initialize(Grid& grid, const ulvec3& size);
void advection (Grid& grid, const ulvec3& size);
void diffusion (Grid& grid, const ulvec3& size);
void projection(Grid& grid, const ulvec3& size);

void forceSolve(Grid& grid, const ulvec3& size);
void pressureSolve(Grid& grid, const ulvec3& size);

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