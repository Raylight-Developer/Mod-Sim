#pragma once

#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <optional>
#include <cstdlib>
#include <cstddef>
#include <iomanip>
#include <fstream>
#include <numeric>
#include <sstream>
#include <variant>
#include <cerrno>
#include <chrono>
#include <future>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <array>
#include <regex>
#include <tuple>
#include <any>
#include <map>
#include <set>

#include <windows.h>
#include <direct.h>
#include <math.h>

#include "Glm.hpp"
#include "Types.hpp"
#include "String.hpp"
#include "Macros.hpp"

#include <QtWidgets>
#include <QtCore>
#include <QtGui>

using namespace std;


struct Particle_Params {
	string name;
	string system_id;
	dvec2 center;
	dvec2 velocity;
	dvec2 acceleration;
	dvec1 restitution;
	dvec1 radius;
	dvec1 mass;
	dvec1 inertia;
	dvec1 angular_velocity;
	bool  colliding;

	Particle_Params(const string& name, const string& system_id, const dvec2& center, const dvec2& velocity, dvec1 restitution, dvec1 radius, dvec1 mass) :
		name(name),
		system_id(system_id),
		center(center),
		velocity(velocity), 
		restitution(restitution),
		radius(radius),
		mass(mass),
		acceleration(dvec2(0.0, 0.0)),
		inertia((2.0 / 5.0) * mass * radius * radius),
		angular_velocity(0.0), 
		colliding(false)
	{}
};