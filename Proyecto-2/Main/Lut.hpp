#pragma once

#include "Shared.hpp"

enum struct Texture_Field {
	NONE,
	TOPOGRAPHY,
	BATHYMETRY,
	SURFACE_PRESSURE,
	SEA_SURFACE_TEMPERATURE_DAY,
	SEA_SURFACE_TEMPERATURE_NIGHT,
	LAND_SURFACE_TEMPERATURE_DAY,
	LAND_SURFACE_TEMPERATURE_NIGHT,

	HUMIDITY,
	WATER_VAPOR,
	CLOUD_COVERAGE,
	CLOUD_WATER_CONTENT,
	CLOUD_PARTICLE_RADIUS,
	CLOUD_OPTICAL_THICKNESS,

	OZONE,
	ALBEDO,
	UV_INDEX,
	NET_RADIATION,
	SOLAR_INSOLATION,
	OUTGOING_LONGWAVE_RADIATION,
	REFLECTED_SHORTWAVE_RADIATION
};

vec1 lut(const Texture_Field& field, const vec1& color) {
	switch (field) {
		case Texture_Field::TOPOGRAPHY:
			if (color >= 1.0f)
				return -1.0f;
			return f_map(0.0f, 1.0f, 0.0f, 6400.0f, color);
		case Texture_Field::BATHYMETRY:
			return f_map(0.0f, 1.0f, 0.0f, -8000.0f, color);
		case Texture_Field::SURFACE_PRESSURE:
			return f_map(0.0f, 1.0f, 800.0f, 1020.0f, color);
		case Texture_Field::SEA_SURFACE_TEMPERATURE_DAY:
		case Texture_Field::SEA_SURFACE_TEMPERATURE_NIGHT:
			return f_map(0.0f, 1.0f, -2.0f, 35.0f, color) + 273.15f;
		case Texture_Field::LAND_SURFACE_TEMPERATURE_DAY:
		case Texture_Field::LAND_SURFACE_TEMPERATURE_NIGHT:
			return f_map(0.0f, 1.0f, -25.0f, 45.0f, color) + 273.15f;
		case Texture_Field::HUMIDITY:
			return f_map(0.0f, 1.0f, 0.1f, 0.9f, color);
		case Texture_Field::WATER_VAPOR:
			return f_map(0.0f, 1.0f, 0.0f, 6.0f, color);
		case Texture_Field::CLOUD_COVERAGE:
			return color;
		case Texture_Field::CLOUD_WATER_CONTENT:
			return f_map(0.0f, 1.0f, 0.0f, 1000.0f, color);
		case Texture_Field::CLOUD_PARTICLE_RADIUS:
			return f_map(0.0f, 1.0f, 4.0f, 40.0f, color);
		case Texture_Field::CLOUD_OPTICAL_THICKNESS:
			return f_map(0.0f, 1.0f, 0.0f, 0.5f, color);
		case Texture_Field::OZONE:
			return f_map(0.0f, 1.0f, 100.0f, 500.0f, color);
		case Texture_Field::ALBEDO:
			return clamp(f_map(0.0f, 1.0f, 0.0f, 0.9f, color), 0.0f, 1.0f);
		case Texture_Field::UV_INDEX:
			return f_map(0.0f, 1.0f, 0.0f, 16.0f, color);
		case Texture_Field::NET_RADIATION:
			return f_map(0.0f, 1.0f, -280.0f, 280.0f, color);
		case Texture_Field::SOLAR_INSOLATION:
			return f_map(0.0f, 1.0f, 0.0f, 550.0f, color);
		case Texture_Field::OUTGOING_LONGWAVE_RADIATION:
			return f_map(0.0f, 1.0f, 85.0f, 350.0f, color);
		case Texture_Field::REFLECTED_SHORTWAVE_RADIATION:
			return f_map(0.0f, 1.0f, 0.0f, 425.0f, color);
	}
	return 0.5f;
}