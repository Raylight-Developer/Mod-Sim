#pragma once

#include "Shared.hpp"

vec1 calculateHeatRate(const vec1& massFlowRate, const vec1& specificHeatCapacity, const vec1& tempIn, const vec1& tempOut);
vec1 calculateMassFlowRate(const vec1& density, const vec1& area, const vec1& velocity);
vec3 coriolisForce(const vec3& velocity, const vec1& mass);
vec1 densityToPresure(const vec1& density, const vec1& target_density);

#define WATER_EMISSIVITY 0.9f
#define ICE_CRYSTAL_EMISSIVITY 0.7f

#define STEFAN_BOLTZMANN_CONSTANT             5.67e-8f // W/m²·K⁴
#define Convective_Heat_Transfer_Coefficient  10.0f // W/m²·K⁴

vec1 calculateConvectiveHeatTransfer(const vec1& surface_area, const vec1& surrounding_temperature, const vec1& particle_temperature);
vec1 calculateRadiativeHeatTransfer(const vec1& emissivity, const vec1& surface_area, const vec1& surrounding_temperature, const vec1& particle_temperature);
vec1 calculateTotalHeatAbsorption(const vec1& surface_area, const vec1& emissivity, const vec1& surrounding_temperature, const vec1& particle_temperature);