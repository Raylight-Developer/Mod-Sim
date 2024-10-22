#pragma once

#include "Shared.hpp"

enum struct Texture_Field {
	NONE,
	TOPOGRAPHY,
	SST, // Sea Surface Temperature
	LST, // Land Surface Temperature
	CLOUD_CONTENT,
	CLOUD_FRACT,
	CLOUD_RAD
};

vec1 lut(const Texture_Field& field, const vec1& color) {
	switch (field) {
		case Texture_Field::TOPOGRAPHY: {
			if (color >= 1.0f) {
				return -1.0f;
			}
			return f_map(0.0f, 1.0f, 0.0f, 6400.0f, color);
		}
		case Texture_Field::SST: {
			return f_map(0.0f, 1.0f, -2.0f, 35.0f, color);
		}
		case Texture_Field::LST: {
			return f_map(0.0f, 1.0f, -25.0f, 45.0f, color);
		}
	}
	return 0.5f;
}