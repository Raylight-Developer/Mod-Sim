#include "Window.hpp"

#undef NL
#undef FILE

#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"

Renderer::Renderer() {
	window = nullptr;
	kernel = Kernel();
	pathtracer = PathTracer(this);

	camera_transform = Transform(dvec3(0, 0, 37.5), dvec3(0));

	runframe = 0;

	frame_counter = 0;
	frame_timer = 0;
	sim_timer = 0;

	frame_count = 0;
	frame_time = 0;
	sim_time = 0;

	display_resolution = uvec2(3840U, 2160U);
	display_aspect_ratio = u_to_d(display_resolution.x) / u_to_d(display_resolution.y);

	camera_zoom_sensitivity = 7.5;
	camera_orbit_sensitivity = 20.0;
	input_lerp = 15.0;
	inputs = vector(6, false);
	input_lerps = vector(6, dvec3(0.0));

	sim_time_aggregate = 0.0;

	current_time = 0.0;
	window_time = 0.0;
	delta_time = FPS_60;
	sim_delta = FPS_60 / 2.0;
	last_time = 0.0;

	run_sim = false;
	lock_settings = false;

	params_float["TIME_SCALE"]          = 1.0f;
	params_float["RENDER_SCALE"]        = 0.5f;
	params_int["PARTICLE_COUNT"]        = 8192;
	params_int["MAX_PARTICLES"]         = 4096 * 4;
	params_int["MAX_OCTREE_DEPTH"]      = 4;

	params_float["POLE_BIAS"]          = 0.9f;
	params_float["POLE_BIAS_POWER"]    = 5.0f;
	params_float["POLE_GEOLOCATION.x"] = 23.1510f;
	params_float["POLE_GEOLOCATION.y"] = 93.0422f;
	params_float["EARTH_TILT"]         = 23.5f;

	params_int["CALENDAR_DAY"]   = 22;
	params_int["CALENDAR_MONTH"] = 12;
	params_float["DATE_TIME"] = dateToFloat(params_int["CALENDAR_MONTH"], params_int["CALENDAR_DAY"]);

	render_resolution = d_to_u(u_to_d(display_resolution) * f_to_d(params_float["RENDER_SCALE"]));
	render_aspect_ratio = u_to_d(render_resolution.x) / u_to_d(render_resolution.y);
}

Renderer::~Renderer() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	pathtracer.f_cleanup();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Renderer::preInit() {
	initGlfw();
	initImGui();
	//systemInfo();

	f_pipeline();
	current_time = 0.0;
	f_displayLoop();
}

void Renderer::initGlfw() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	display_resolution = uvec2(mode->width, mode->height);
	display_aspect_ratio = u_to_d(display_resolution.x) / u_to_d(display_resolution.y);

	window = glfwCreateWindow(display_resolution.x, display_resolution.y, "Screensaver", NULL, NULL);

	if (window == NULL) {
		cout << "Failed to create GLFW window" << endl;
		glfwTerminate();
	}

	int width, height, channels;
	unsigned char* image = stbi_load("Resources/Logo.png", &width, &height, &channels, 4);
	if (!image) {
		cerr << "Failed to load icon image" << std::endl;
	} else {
		GLFWimage icon;
		icon.width = width;
		icon.height = height;
		icon.pixels = image;

		glfwSetWindowIcon(window, 1, &icon);
		stbi_image_free(image);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(true);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	glfwSetWindowUserPointer(window, this);

	glfwSetFramebufferSizeCallback(window, framebufferSize);
	glfwSetMouseButtonCallback(window, mouseButton);
	glfwSetScrollCallback(window, scroll);
	glfwSetKeyCallback(window, key);
}

void Renderer::initImGui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = nullptr;
	io.Fonts->AddFontFromFileTTF("./Resources/RobotoMono-Medium.ttf", 18.0f);
	io.FontGlobalScale = f_map((1920.0f * 1080.0f), (3840.0f * 2160.0f), 1.0f, 2.25f, vec1(display_resolution . x * display_resolution.y));

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

void Renderer::systemInfo() {
	GLint work_grp_cnt[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
	LOG << "Max work groups per compute shader" <<
		" x:" << work_grp_cnt[0] <<
		" y:" << work_grp_cnt[1] <<
		" z:" << work_grp_cnt[2];

	GLint work_grp_size[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
	LOG ENDL << "Max work group sizes" <<
		" x:" << work_grp_size[0] <<
		" y:" << work_grp_size[1] <<
		" z:" << work_grp_size[2];

	GLint work_grp_inv;
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	LOG ENDL << "Max invocations count per work group: " << work_grp_inv;

	GLint uboMaxSize;
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE , &uboMaxSize);
	LOG ENDL << "Maximum UBO size: " << d_to_ul(round(i_to_d(uboMaxSize) / (1024.0 * 1024.0))) << " Mb";

	GLint ssboMaxSize;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &ssboMaxSize);
	LOG ENDL << "Maximum SSBO size per binding: " << d_to_ul(round(i_to_d(ssboMaxSize) / (1024.0 * 1024.0))) << " Mb";

	GLint maxSSBOBindings;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxSSBOBindings);
	LOG ENDL << "Maximum SSBO bindings: " << maxSSBOBindings;

	GLint uniformBufferOffsetAlignment;
	glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &uniformBufferOffsetAlignment);
	LOG ENDL << "SSBO struct alignment multiplier: " << uniformBufferOffsetAlignment;
	LOG ENDL;
	FLUSH;
}

void Renderer::f_pipeline() {
	pathtracer.f_initialize();
	f_changeSettings();
}

void Renderer::f_recompile() {
	pathtracer.f_recompile();
}

void Renderer::f_tickUpdate() {
	if (run_sim) {
		const dvec1 start = glfwGetTime();
		const dvec1 delta = delta_time * params_float["TIME_SCALE"];
		vec1* date_time = &params_float.at("DATE_TIME");
		*date_time -= d_to_f(delta) * 0.05f;
		if (*date_time < 0.0f) {
			*date_time += 1.0f;
		}
		kernel.simulate(delta, *date_time);
		sim_delta = glfwGetTime() - start;
		pathtracer.f_tickUpdate();
	}
	else {
		sim_delta = 0.0;
	}
}

void Renderer::f_changeSettings() {
	kernel.preInit(params_float, params_bool, params_int);
	pathtracer.f_changeSettings();
}

void Renderer::f_guiLoop() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::Begin("Info");

	ImGui::SeparatorText("Average Stats");
	{
		const dvec1 percent = round((sim_time_aggregate / current_time) * 100.0);
		ImGui::Text(("~GPU[" + to_str(100.0 - percent, 0) + "]%%").c_str());
		ImGui::Text(("~CPU[" + to_str(percent, 0) + "]%%").c_str());

		ImGui::Text(("Fps: " + to_str(ul_to_d(runframe) / current_time, 0)).c_str());
	}
	ImGui::SeparatorText("Stats");
	{
		const dvec1 percent = round((sim_time / frame_time) * 100.0);
		ImGui::Text(("~GPU[" + to_str(100.0 - percent, 0) + "]%%").c_str());
		ImGui::Text(("~CPU[" + to_str(percent, 0) + "]%%").c_str());

		ImGui::Text(("Fps: " + to_str(frame_count, 0)).c_str());
	}
	ImGui::Text(("Zoom: " + to_str(camera_zoom_sensitivity, 1) + "    Orbit: " + to_str(camera_orbit_sensitivity, 1)).c_str());
	ImGui::SeparatorText("Play / Pause");
	if (run_sim) {
		if (ImGui::Button("Pause")) {
			run_sim = false;
		}
	}
	else {
		if (lock_settings) {
			if (ImGui::Button("Play")) {
				run_sim = true;
			}
		}
		else {
			if (ImGui::Button("Lock n Load Settings")) {
				lock_settings = true;
				f_changeSettings();
				kernel.init();
			}
			ImGui::SeparatorText("Earth Settings");
			if (ImGui::SliderFloat("Latitude", &params_float["POLE_GEOLOCATION.x"], -90.0f, 90.0f, "%.4f")) {
				f_changeSettings();
			}
			if (ImGui::SliderFloat("Longitude", &params_float["POLE_GEOLOCATION.y"], -180.0f, 180.0f, "%.4f")) {
				f_changeSettings();
			}
			if (ImGui::SliderFloat("Pole Bias", &params_float["POLE_BIAS"], 0.0f, 1.0f, "%.5f")) {
				f_changeSettings();
			}
			if (ImGui::SliderFloat("Pole Power", &params_float["POLE_BIAS_POWER"], 1.0f, 10.0f)) {
				f_changeSettings();
			}
			if (ImGui::SliderFloat("Earth Tilt", &params_float["EARTH_TILT"], -180.0f, 180.0f, "%.2f")) {
				f_changeSettings();
			}

			if (ImGui::SliderInt("Day", &params_int["CALENDAR_DAY"], 0, 31)) {
				params_float["DATE_TIME"] = dateToFloat(params_int["CALENDAR_MONTH"], params_int["CALENDAR_DAY"]);
				f_changeSettings();
			}
			if (ImGui::SliderInt("Month", &params_int["CALENDAR_MONTH"], 0, 12)) {
				params_float["DATE_TIME"] = dateToFloat(params_int["CALENDAR_MONTH"], params_int["CALENDAR_DAY"]);
				f_changeSettings();
			}

			ImGui::SeparatorText("Simulation Settings");

			if (ImGui::SliderInt("Particle Count", &params_int["PARTICLE_COUNT"], 128, params_int["MAX_PARTICLES"])) {
				f_changeSettings();
			}
			ImGui::SliderFloat("Time Scale", &params_float["TIME_SCALE"], 0.01f, 2.0f, "%.4f");
		}
	}

	pathtracer.f_guiUpdate();

	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::f_frameUpdate() {
	sim_time_aggregate += sim_delta;
	frame_timer += delta_time;
	sim_timer += sim_delta;
	frame_counter++;
	runframe++;

	if (window_time > 1.0) {
		frame_count = frame_counter;
		frame_time = frame_timer;
		sim_time = sim_timer;

		frame_counter = 0;
		frame_timer = 0.0;
		sim_timer = 0.0;
		window_time -= 1.0;
	}
}

void Renderer::f_inputLoop() {
	for (uint i = 0; i < 6; i++) {
		dvec3* input = &input_lerps[i];
		if (input->y < 1.0) {
			input->y += delta_time / input_lerp;
			if (input->y > 1.0)
				input->y = 1.0;
			input->x = lerp(input->x, input->z, input->y);
		}
		if (i == 0) {
			camera_transform.orbit(dvec3(0), dvec2(-1, 0) * input->x * camera_orbit_sensitivity * delta_time);
		}
		if (i == 1) {
			camera_transform.orbit(dvec3(0), dvec2(0, -1) * input->x * camera_orbit_sensitivity * delta_time);
		}
		if (i == 2) {
			camera_transform.orbit(dvec3(0), dvec2(1, 0) * input->x * camera_orbit_sensitivity * delta_time);
		}
		if (i == 3) {
			camera_transform.orbit(dvec3(0), dvec2(0, 1) * input->x * camera_orbit_sensitivity * delta_time);
		}
		if (i == 4) {
			camera_transform.moveLocal(dvec3(0, 0, -1) * input->x * camera_zoom_sensitivity * delta_time);
		}
		if (i == 5) {
			camera_transform.moveLocal(dvec3(0, 0, 1) * input->x * camera_zoom_sensitivity * delta_time);
		}
	}
}

void Renderer::f_timings() {
	current_time = glfwGetTime();
	delta_time = current_time - last_time;
	last_time = current_time;
	window_time += delta_time;
}

void Renderer::f_displayLoop() {
	while (!glfwWindowShouldClose(window)) {

		f_timings();
		f_inputLoop();
		f_tickUpdate();
		pathtracer.f_render();

		f_frameUpdate();
		f_guiLoop();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void Renderer::f_resize() {
	display_aspect_ratio = u_to_d(display_resolution.x) / u_to_d(display_resolution.y);
	render_resolution = d_to_u(u_to_d(display_resolution) * f_to_d(params_float["RENDER_SCALE"]));
	render_aspect_ratio = u_to_d(render_resolution.x) / u_to_d(render_resolution.y);
	pathtracer.f_resize();
}

void Renderer::framebufferSize(GLFWwindow* window, int width, int height) {
	Renderer* instance = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	glViewport(0, 0, width, height);
	instance->display_resolution.x = width;
	instance->display_resolution.y = height;
	instance->f_resize();
}

void Renderer::mouseButton(GLFWwindow* window, int button, int action, int mods) {
	Renderer* instance = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	if (action == GLFW_PRESS) {
	}
	else if (action == GLFW_RELEASE) {
	}
}

void Renderer::scroll(GLFWwindow* window, double xoffset, double yoffset) {
	Renderer* instance = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
}

void Renderer::key(GLFWwindow* window, int key, int scancode, int action, int mods) {
	Renderer* instance = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	// Input Handling
	if (key == GLFW_KEY_F5 && action == GLFW_PRESS) {
		instance->f_recompile();
	}
	//if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
	//	glfwSetWindowShouldClose(window, GLFW_TRUE);
	//}
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		instance->camera_orbit_sensitivity /= 0.9;
		instance->camera_zoom_sensitivity /= 0.9;
	}
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		instance->camera_orbit_sensitivity *= 0.9;
		instance->camera_zoom_sensitivity *= 0.9;
	}
	uint64 rkey = 6;
	switch (key) {
		case GLFW_KEY_W: rkey = 0; break;
		case GLFW_KEY_A: rkey = 1; break;
		case GLFW_KEY_S: rkey = 2; break;
		case GLFW_KEY_D: rkey = 3; break;
		case GLFW_KEY_UP: rkey = 4; break;
		case GLFW_KEY_DOWN: rkey = 5; break;
	}

	if (rkey != 6) {
		if (action == GLFW_PRESS && !instance->inputs[rkey]) { // Key was just pressed
			instance->inputs[rkey] = true;
			instance->input_lerps[rkey].y = 0.0; // Progress
			instance->input_lerps[rkey].z = 1.0; // Target
		}
		else if (action == GLFW_RELEASE && instance->inputs[rkey]) { // Key was just released
			instance->inputs[rkey] = false;
			instance->input_lerps[rkey].y = 0.0; // Progress
			instance->input_lerps[rkey].z = 0.0; // Target
		}
	}
}