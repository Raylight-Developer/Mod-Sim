#pragma execution_character_set( "utf-8" )

#include "Include.hpp"

#include "Window.hpp"

int main(int argc, char* argv[]) {
	SetConsoleOutputCP(65001);

	vec1  sphereRadius = 0.015f;
	vec1  sphereDisplayRadius = sphereRadius;
	uint  particleCount = 128;
	vec1  renderScale = 0.5f;
	bool  openmp = false;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--voxel-size") == 0 && i + 1 < argc) {
			sphereRadius = str_to_f(argv[++i]);
		} else if (strcmp(argv[i], "--sphere-display-mult") == 0 && i + 1 < argc) {
			sphereDisplayRadius = sphereRadius * str_to_f(argv[++i]);
		} else if (strcmp(argv[i], "--grid-size") == 0 && i + 2 < argc) {
			particleCount = str_to_u(argv[++i]);
		}else if (strcmp(argv[i], "--render-scale") == 0 && i + 1 < argc) {
			renderScale = str_to_f(argv[++i]);
		} else if (strcmp(argv[i], "--openmp") == 0 && i + 1 < argc) {
			openmp = bool(str_to_i(argv[++i]));
		} else {
			cerr << "Unknown or incomplete argument: " << argv[i] << endl;
		}
	}

	Renderer renderer(sphereRadius, sphereDisplayRadius, particleCount, renderScale, openmp);
	renderer.init();

	return 1;
}