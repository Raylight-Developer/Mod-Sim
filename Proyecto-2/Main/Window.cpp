#include "Window.hpp"

#undef NL
#undef FILE

#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"

Renderer::Renderer() {
	window = nullptr;
	kernel = Kernel();

	rasterizer = Rasterizer(this);
	pathtracer = PathTracer(this);

	camera_transform = Transform(dvec3(0, 0, 37.5), dvec3(0));
	camera_transform.orbit(dvec3(0), dvec3(-15, 15, 0));

	render_mode = Mode::RASTERIZATION;

	runframe = 0;

	frame_counter = 0;
	frame_timer = 0;
	sim_timer = 0;

	frame_count = 0;
	frame_time = 0;
	sim_time = 0;

	display_resolution = uvec2(3840U, 2160U);
	display_aspect_ratio = u_to_d(display_resolution.x) / u_to_d(display_resolution.y);

	camera_zoom_sensitivity = 2.0;
	camera_orbit_sensitivity = 5.0;
	inputs = vector(348, false);

	sim_time_aggregate = 0.0;

	current_time = 0.0;
	window_time = 0.0;
	delta_time = FPS_60;
	sim_delta = FPS_60 / 2.0;
	last_time = 0.0;

	run_sim = false;

	TIME_SCALE       = 0.1f;
	RENDER_SCALE     = 0.25f;
	PARTICLE_RADIUS  = 0.065f;
	PARTICLE_COUNT   = 1024;
	MAX_PARTICLES    = 4096 * 4;
	LAYER_COUNT      = 1;
	PARTICLE_DISPLAY = 1.0f;
	MAX_OCTREE_DEPTH = 2;

	POLE_BIAS = 0.95f;
	POLE_BIAS_POWER = 2.5f;
	POLE_GEOLOCATION = vec2(23.1510, 93.0422);

	render_resolution = d_to_u(u_to_d(display_resolution) * f_to_d(RENDER_SCALE));
	render_aspect_ratio = u_to_d(render_resolution.x) / u_to_d(render_resolution.y);
}

Renderer::~Renderer() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Renderer::init() {
	initGlfw();
	initImGui();
	systemInfo();

	f_pipeline();
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
	if (render_mode == Mode::PATHTRACING) {
		render_mode = Mode::RASTERIZATION;
		pathtracer.f_cleanup();
		rasterizer.f_initialize();
	}
	else if (render_mode == Mode::RASTERIZATION) {
		render_mode = Mode::PATHTRACING;
		rasterizer.f_cleanup();
		pathtracer.f_initialize();
	}
	f_changeSettings();
}

void Renderer::f_recompile() {
	pathtracer.f_recompile();
	rasterizer.f_recompile();
}

void Renderer::f_tickUpdate() {
	if (run_sim) {
		const dvec1 start = glfwGetTime();
		const dvec1 delta = delta_time * TIME_SCALE;
		kernel.simulate(delta);
		sim_delta = glfwGetTime() - start;
	}
	else {
		sim_delta = 0.0;
	}
}

void Renderer::f_changeSettings() {
	kernel.init(PARTICLE_RADIUS, PARTICLE_COUNT, LAYER_COUNT, MAX_OCTREE_DEPTH, POLE_BIAS, POLE_BIAS_POWER, POLE_GEOLOCATION);

	if (render_mode == Mode::PATHTRACING) {
		pathtracer.f_changeSettings();
	}
	else if (render_mode == Mode::RASTERIZATION) {
		//rasterizer.f_resize();
	}
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
	ImGui::SeparatorText("General Settings");
	ImGui::SliderFloat("Time Scale", &TIME_SCALE, 0.01f, 2.0f, "%.4f");
	if (ImGui::SliderFloat("Render Scale", &RENDER_SCALE, 0.1f, 1.0f, "%.3f")) {
		f_resize();
	}
	ImGui::SeparatorText("Play / Pause");
	if (run_sim) {
		if (ImGui::Button("Stop")) {
			run_sim = false;
		}
		if (ImGui::Button("Restart")) {
			run_sim = false;
			f_changeSettings();
		}
	}
	else {
		if (ImGui::Button("Start")) {
			run_sim = true;
		}
		ImGui::SeparatorText("Init Settings");
		if (ImGui::SliderFloat("Particle Radius", &PARTICLE_RADIUS, 0.001f, 1.0f, "%.5f")) {
			f_changeSettings();
		}

		if (ImGui::SliderFloat("Display Mult", &PARTICLE_DISPLAY, 0.05f, 5.0f, "%.3f")) {
			f_changeSettings();
		}

		if (ImGui::SliderInt("Particle Count", &PARTICLE_COUNT, 128, MAX_PARTICLES)) {
			f_changeSettings();
		}

		if (ImGui::SliderInt("Max Octree Depth", &MAX_OCTREE_DEPTH, 0, 10)) {
			f_changeSettings();
		}

		if (ImGui::SliderInt("Layer Count", &LAYER_COUNT, 1, 16)) {
			f_changeSettings();
		}

		if (ImGui::SliderFloat("Pole Bias", &POLE_BIAS, 0.0f, 1.0f, "%.5f")) {
			f_changeSettings();
		}

		if (ImGui::SliderFloat("Pole Power", &POLE_BIAS_POWER, 1.0f, 3.0f)) {
			f_changeSettings();
		}

		if (ImGui::SliderFloat("Latitude", &POLE_GEOLOCATION.x, -90.0f, 90.0f), "%.4f") {
			f_changeSettings();
		}
		if (ImGui::SliderFloat("Longitude", &POLE_GEOLOCATION.y, -180.0f, 180.0f, "%.4f")) {
			f_changeSettings();
		}
	}

	if (render_mode == Mode::PATHTRACING) {
		pathtracer.f_guiUpdate();
	}
	else if (render_mode == Mode::RASTERIZATION) {
		rasterizer.f_guiUpdate();
	}

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
	//if (inputs[GLFW_MOUSE_BUTTON_RIGHT] or inputs[GLFW_MOUSE_BUTTON_LEFT]) {
	//	const dvec1 xoffset = (last_mouse.x - current_mouse.x) * delta_time * camera_orbit_sensitivity;
	//	const dvec1 yoffset = (last_mouse.y - current_mouse.y) * delta_time * camera_orbit_sensitivity;
	//	camera_transform.orbit(dvec3(0.0), dvec2(yoffset, xoffset));
	//	last_mouse = current_mouse;
	//}
	if (inputs[GLFW_KEY_W]) {
		camera_transform.orbit(dvec3(0.0), dvec2(-25, 0) * delta_time);
	}
	if (inputs[GLFW_KEY_A]) {
		camera_transform.orbit(dvec3(0.0), dvec2(0, -25) * delta_time);
	}
	if (inputs[GLFW_KEY_S]) {
		camera_transform.orbit(dvec3(0.0), dvec2(25, 0) * delta_time);
	}
	if (inputs[GLFW_KEY_D]) {
		camera_transform.orbit(dvec3(0.0), dvec2(0, 25) * delta_time);
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
		if (render_mode == Mode::PATHTRACING) {
			f_inputLoop();
			f_tickUpdate();
			pathtracer.f_render();
		}
		else if (render_mode == Mode::RASTERIZATION) {
			f_inputLoop();
			f_tickUpdate();
			rasterizer.f_render();
		}


		f_frameUpdate();
		f_guiLoop();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void Renderer::f_resize() {
	display_aspect_ratio = u_to_d(display_resolution.x) / u_to_d(display_resolution.y);
	render_resolution = d_to_u(u_to_d(display_resolution) * f_to_d(RENDER_SCALE));
	render_aspect_ratio = u_to_d(render_resolution.x) / u_to_d(render_resolution.y);
	if (render_mode == Mode::PATHTRACING) {
		pathtracer.f_resize();
	}
	else if (render_mode == Mode::RASTERIZATION) {
		rasterizer.f_resize();
	}
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
		instance->inputs[button] = true;
	}
	else if (action == GLFW_RELEASE) {
		instance->inputs[button] = false;
	}
}

void Renderer::scroll(GLFWwindow* window, double xoffset, double yoffset) {
	Renderer* instance = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	if (yoffset < 0) {
		instance->camera_transform.moveLocal(dvec3(0.0, 0.0,  25.0) * instance->camera_zoom_sensitivity * instance->delta_time);
	}
	if (yoffset > 0) {
		instance->camera_transform.moveLocal(dvec3(0.0, 0.0, -25.0) * instance->camera_zoom_sensitivity * instance->delta_time);
	}
}

void Renderer::key(GLFWwindow* window, int key, int scancode, int action, int mods) {
	Renderer* instance = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	// Input Handling
	if (key == GLFW_KEY_F5 && action == GLFW_PRESS) {
		instance->f_recompile();
	}
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	if (key == GLFW_KEY_M && action == GLFW_PRESS) {
		instance->f_pipeline();
	}
	if (action == GLFW_PRESS) {
		instance->inputs[key] = true;
	}
	else if (action == GLFW_RELEASE) {
		instance->inputs[key] = false;
	}
}