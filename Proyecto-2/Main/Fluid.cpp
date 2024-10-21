#include "Fluid.hpp"

vec1 calculateHeatRate(const vec1& massFlowRate, const vec1& specificHeatCapacity, const vec1& tempIn, const vec1& tempOut) {
	return massFlowRate * specificHeatCapacity * (tempIn - tempOut);
}

vec1 calculateMassFlowRate(const vec1& density, const vec1& area, const vec1& velocity) {
	return density * area * velocity;
}

vec3 coriolisForce(const vec3& velocity, const vec1& mass) {
	return glm::cross(vec3(0.0, 0.0, 7.2921159e-5f), velocity) * (-2.0f * mass);
}

vec1 densityToPresure(const vec1& density, const vec1& target_density) {
	return density - target_density;
}

vec1 calculateConvectiveHeatTransfer(const vec1& surface_area, const vec1& surrounding_temperature, const vec1& particle_temperature) {
	return Convective_Heat_Transfer_Coefficient * surface_area * (surrounding_temperature - particle_temperature);
}

vec1 calculateRadiativeHeatTransfer(const vec1& emissivity, const vec1& surface_area, const vec1& surrounding_temperature, const vec1& particle_temperature) {
	return emissivity * STEFAN_BOLTZMANN_CONSTANT * surface_area * (pow(surrounding_temperature, 4.0f) - pow(particle_temperature, 4.0f));
}

vec1 calculateTotalHeatAbsorption(const vec1& surface_area, const vec1& emissivity, const vec1& surrounding_temperature, const vec1& particle_temperature) {
	const vec1 Q_conv = calculateConvectiveHeatTransfer(surface_area, surrounding_temperature, particle_temperature);
	const vec1 Q_rad = calculateRadiativeHeatTransfer(emissivity, surface_area, surrounding_temperature, particle_temperature);
	return Q_conv + Q_rad;
}
