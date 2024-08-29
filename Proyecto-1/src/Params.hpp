#pragma once

#include "Include.hpp"

#define BOUNDING_BOX QRectF(-200, 0, 400, 800)

const vector<Particle_Params> PARAMETERS = {
	Particle_Params("0", "-1", dvec2(0, 185), dvec2(0, 0), 0.8, 5.0 , 2.0 ),
	Particle_Params("1", "-1", dvec2(0, 165), dvec2(0, 0), 0.8, 10.0, 10.0),
	Particle_Params("2", "-1", dvec2(0, 130), dvec2(0, 0), 0.8, 20.0, 25.0),
	Particle_Params("3", "-1", dvec2(0, 80 ), dvec2(0, 0), 0.7, 30.0, 40.0)
};