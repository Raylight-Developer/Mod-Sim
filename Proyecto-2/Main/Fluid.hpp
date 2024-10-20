#pragma once

#include "Shared.hpp"

vec1 calculateHeatRate(vec1 massFlowRate, vec1 specificHeatCapacity, vec1 tempIn, vec1 tempOut) {
	return massFlowRate * specificHeatCapacity * (tempIn - tempOut);
}

vec1 calculateMassFlowRate(vec1 density, vec1 area, vec1 velocity) {
	return density * area * velocity;
}

vec3 coriolisForce(vec3 velocity, vec1 mass) {
	return glm::cross(vec3(0.0, 0.0, 7.2921159e-5f), velocity) * (-2.0f * mass) ;
}

vec1 densityToPresure(vec1 density, vec1 target_density) {
	return density - target_density;
}