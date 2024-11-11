#include "Window.hpp"

#undef NL
#undef FILE

#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"

Renderer::Renderer() {
	window = nullptr;
	kernel = Kernel();
	pathtracer = PathTracer(this);
	if (pathtracer.render_probe_color_mode < SPH) {
		kernel.BVH_SPH = false;
	}
	else {
		kernel.BVH_SPH = true;
	}

	camera_transform = Transform(dvec3(0, 0, 37.5), dvec3(0));
	world_rot = dquat(1,0,0,0);
	camera = dquat(1,0,0,0);
	zoom = 37.5;

	runframe = 0;

	frame_counter = 0;
	frame_timer = 0;
	gpu_timer = 0;

	frame_count = 0;
	frame_time = 0;
	gpu_time = 0;

	display_resolution = uvec2(3840U, 2160U);
	display_aspect_ratio = u_to_d(display_resolution.x) / u_to_d(display_resolution.y);
	render_scale = 0.5f;

	render_resolution = d_to_u(u_to_d(display_resolution) * f_to_d(render_scale));
	render_aspect_ratio = u_to_d(render_resolution.x) / u_to_d(render_resolution.y);

	camera_zoom_sensitivity = 7.5;
	camera_orbit_sensitivity = 0.05;
	input_lerp = 10.0;
	inputs = vector(6, false);
	input_lerps = vector(6, dvec3(0.0));

	gpu_time_aggregate = 0.0;

	current_time = chrono::high_resolution_clock::now();
	window_time = 0.0;
	delta_time = FPS_60;
	gpu_delta = FPS_60 / 2.0;
	last_time = current_time;

	run_sim = false;
	next_frame = false;
	lock_settings = false;
	lock_view = false;
	
	INIT_TIMER("Probe BVH")
	INIT_TIMER("Particle BVH")
	INIT_TIMER("Particle Update")
	INIT_TIMER("Scatter")
	INIT_TIMER("Gather")
	INIT_TIMER("GUI Loop")
	INIT_TIMER("GPU")
	INIT_TIMER("CPU")
	INIT_TIMER("Delta")
	INIT_TIMER("Transfer")
}

Renderer::~Renderer() {
	pathtracer.f_cleanup();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Renderer::init() {
	initGlfw();
	initImGui();
	//systemInfo();

	f_pipeline();
	current_time = chrono::high_resolution_clock::now();
	start_time = chrono::high_resolution_clock::now();
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
		cerr << "Failed to create GLFW window" << endl;
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
	f_updateProbes();
	f_updateParticles();
}

void Renderer::f_tickUpdate() {
	TIMER("Probe BVH") = 0.0;
	TIMER("Particle BVH") = 0.0;
	TIMER("Scatter") = 0.0;
	TIMER("Gather") = 0.0;
	TIMER("Particle Update") = 0.0;
	TIMER("Transfer") = 0.0;
	if (run_sim or next_frame) {
		if (next_frame) {
			kernel.simulate(FPS_60);
		}
		else {
			kernel.simulate(delta_time);
		}

		//kernel.buildProbes();
		//kernel.buildParticles();

		pathtracer.f_updateProbes();
		pathtracer.f_updateParticles();

		next_frame = false;
	}
}

void Renderer::f_updateProbes() {
	kernel.buildProbes();
	kernel.updateGPUProbes();
	pathtracer.f_updateProbes();
}

void Renderer::f_updateParticles() {
	kernel.buildParticles();
	kernel.updateGPUParticles();
	pathtracer.f_updateParticles();
}

void Renderer::f_guiLoop() {
	START_TIMER("GUI Loop");
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(1000, FLT_MAX));
	ImGui::Begin("Info");
	if (lock_view) {
		ImGui::Text("Locked View");
	}
	//ImGui::SetWindowPos(ImVec2(0, 0));
	const vec1 availableWidth = ImGui::GetContentRegionAvail().x;
	const vec1 spacing = ImGui::GetStyle().ItemSpacing.x;
	const vec1 itemWidth = availableWidth;
	const vec1 halfWidth = itemWidth * 0.5f - spacing * 0.5f;
	const vec1 thirdWidth = itemWidth / 3.0f - spacing / 3.0f;
	const vec1 halfPos = halfWidth + spacing * 2.0f;
	const vec1 thirdPos = thirdWidth + spacing * 2.0f;

	ImGui::PushItemWidth(itemWidth);
	ImGui::SeparatorText("Sampling Settings");
	ImGui::Text("Time Scale");
	float TIME_SCALE = d_to_f(kernel.TIME_SCALE);
	if (ImGui::SliderFloat("##time_scale", &TIME_SCALE, 0.01f, 3650.0f, "%.3f")) {
		kernel.TIME_SCALE = f_to_d(TIME_SCALE);
	}

	int SAMPLES = kernel.SUB_SAMPLES;
	ImGui::Text("Sub Samples");
	if (ImGui::SliderInt("##samples", &SAMPLES, 1, 50)) {
		kernel.SUB_SAMPLES = SAMPLES;
	}
	ImGui::PopItemWidth();

	ImGui::SeparatorText("Play / Pause");
	if (run_sim) {
		if (ImGui::Button("Pause", ImVec2(itemWidth, 0))) {
			run_sim = false;
		}
	}
	else {
		if (lock_settings) {
			if (ImGui::Button("Play", ImVec2(halfWidth, 0))) {
				run_sim = true;
			}
			ImGui::SameLine();
			ImGui::SetCursorPosX(halfPos);
			if (ImGui::Button("Next Frame", ImVec2(halfWidth, 0))) {
				next_frame = true;
			}
		}
		else {
			if (ImGui::Button("Lock n Load Settings", ImVec2(itemWidth, 0))) {
				lock_settings = true;
				kernel.lock();
				next_frame = true;
			}

			if (ImGui::CollapsingHeader("Probe Settings")) {
				ImGui::PushItemWidth(itemWidth);
				int PROBE_COUNT = u_to_i(kernel.PROBE_COUNT);
				ImGui::Text("Probe Count");
				if (ImGui::SliderInt("##PROBE_COUNT", &PROBE_COUNT, 128, 8192 * 4)) {
					kernel.PROBE_COUNT = i_to_u(PROBE_COUNT);
					f_updateProbes();
				}
				ImGui::PopItemWidth();
				ImGui::SeparatorText("Earth Settings");
				ImGui::PushItemWidth(halfWidth);

				ImGui::Text("Latitude");
				ImGui::SameLine();
				ImGui::SetCursorPosX(halfPos);
				ImGui::Text("Longitude");
				float lon = d_to_f(kernel.PROBE_POLE_GEOLOCATION.x);
				if (ImGui::SliderFloat("##PROBE_POLE_GEOLOCATION_x", &lon, -90.0f, 90.0f, "%.4f")) {
					kernel.PROBE_POLE_GEOLOCATION.x = f_to_d(lon);
					f_updateProbes();
				}
				ImGui::SameLine();
				float lat = d_to_f(kernel.PROBE_POLE_GEOLOCATION.y);
				if (ImGui::SliderFloat("##PROBE_POLE_GEOLOCATION_y", &lat, -180.0f, 180.0f, "%.4f")) {
					kernel.PROBE_POLE_GEOLOCATION.y = f_to_d(lat);
					f_updateProbes();
				}

				ImGui::Text("Pole Bias");
				ImGui::SameLine();
				ImGui::SetCursorPosX(halfPos);
				ImGui::Text("Pole Power");
				float pole_bias = d_to_f(kernel.PROBE_POLE_BIAS);
				if (ImGui::SliderFloat("##PROBE_POLE_BIAS", &pole_bias, 0.0f, 1.0f, "%.5f")) {
					kernel.PROBE_POLE_BIAS = f_to_d(pole_bias);
					f_updateProbes();
				}
				ImGui::SameLine();
				float pole_power = d_to_f(kernel.PROBE_POLE_BIAS_POWER);
				if (ImGui::SliderFloat("##PROBE_POLE_BIAS_POWER", &pole_power, 1.0f, 10.0f)) {
					kernel.PROBE_POLE_BIAS_POWER = f_to_d(pole_power);
					f_updateProbes();
				}
			}
			if (ImGui::CollapsingHeader("Particle Settings")) {
				ImGui::PushItemWidth(itemWidth);
				int PARTICLE_COUNT = u_to_i(kernel.PARTICLE_COUNT);
				ImGui::Text("Particle Count");
				if (ImGui::SliderInt("##PARTICLE_COUNT", &PARTICLE_COUNT, 128, 8192 * 4)) {
					kernel.PARTICLE_COUNT = i_to_u(PARTICLE_COUNT);
					f_updateParticles();
				}
				ImGui::PopItemWidth();
				ImGui::SeparatorText("Earth Settings");
				ImGui::PushItemWidth(halfWidth);

				ImGui::Text("Latitude");
				ImGui::SameLine();
				ImGui::SetCursorPosX(halfPos);
				ImGui::Text("Longitude");
				float lon = d_to_f(kernel.PARTICLE_POLE_GEOLOCATION.x);
				if (ImGui::SliderFloat("##PARTICLE_POLE_GEOLOCATION_x", &lon, -90.0f, 90.0f, "%.4f")) {
					kernel.PARTICLE_POLE_GEOLOCATION.x = f_to_d(lon);
					f_updateParticles();
				}
				ImGui::SameLine();
				float lat = d_to_f(kernel.PARTICLE_POLE_GEOLOCATION.y);
				if (ImGui::SliderFloat("##PARTICLE_POLE_GEOLOCATION_y", &lat, -180.0f, 180.0f, "%.4f")) {
					kernel.PARTICLE_POLE_GEOLOCATION.y = f_to_d(lat);
					f_updateParticles();
				}

				ImGui::Text("Pole Bias");
				ImGui::SameLine();
				ImGui::SetCursorPosX(halfPos);
				ImGui::Text("Pole Power");
				float pole_bias = d_to_f(kernel.PARTICLE_POLE_BIAS);
				if (ImGui::SliderFloat("##PARTICLE_POLE_BIAS", &pole_bias, 0.0f, 1.0f, "%.5f")) {
					kernel.PARTICLE_POLE_BIAS = f_to_d(pole_bias);
					f_updateParticles();
				}
				ImGui::SameLine();
				float pole_power = d_to_f(kernel.PARTICLE_POLE_BIAS_POWER);
				if (ImGui::SliderFloat("##PARTICLE_POLE_BIAS_POWER", &pole_power, 1.0f, 10.0f)) {
					kernel.PARTICLE_POLE_BIAS_POWER = f_to_d(pole_power);
					f_updateParticles();
				}
			}

			ImGui::PopItemWidth();
			ImGui::PushItemWidth(itemWidth);
			ImGui::Text("Earth Tilt");
			float tilt = d_to_f(kernel.EARTH_TILT);
			if (ImGui::SliderFloat("##EARTH_TILT", &tilt, -180.0f, 180.0f, "%.2f")) {
				kernel.EARTH_TILT = f_to_d(tilt);
				f_updateProbes();
				f_updateParticles();
			}
			ImGui::PopItemWidth();
			ImGui::PushItemWidth(halfWidth);

			ImGui::Text("Month");
			ImGui::SameLine();
			ImGui::Text("Day");
			if (ImGui::SliderInt("##CALENDAR_MONTH", &kernel.CALENDAR_MONTH, 0, 12)) {
				kernel.calculateDateTime();
				f_updateProbes();
				f_updateParticles();
			}
			ImGui::SameLine();
			if (ImGui::SliderInt("##CALENDAR_DAY", &kernel.CALENDAR_DAY, 0, 31)) {
				kernel.calculateDateTime();
				f_updateProbes();
				f_updateParticles();
			}

			ImGui::Text("Hour");
			ImGui::SameLine();
			ImGui::SetCursorPosX(halfPos);
			ImGui::Text("Minute");
			if (ImGui::SliderInt("##CALENDAR_HOUR", &kernel.CALENDAR_HOUR, 0, 24)) {
				kernel.calculateDateTime();
				f_updateProbes();
				f_updateParticles();
			}
			ImGui::SameLine();
			if (ImGui::SliderInt("##CALENDAR_MINUTE", &kernel.CALENDAR_MINUTE, 0, 60)) {
				kernel.calculateDateTime();
				f_updateProbes();
				f_updateParticles();
			}

			ImGui::PopItemWidth();
		}
	}
	if (ImGui::CollapsingHeader("Info")) {
		ImGui::PushItemWidth(halfWidth);
		ImGui::Text(("Month:  " + to_str(kernel.CALENDAR_MONTH, 0)).c_str());
		ImGui::SameLine();
		ImGui::SetCursorPosX(halfPos);
		ImGui::Text(("Day:    " + to_str(kernel.CALENDAR_DAY, 0)).c_str());
		ImGui::Text(("Hour:   " + to_str(kernel.CALENDAR_HOUR, 0)).c_str());
		ImGui::SameLine();
		ImGui::SetCursorPosX(halfPos);
		ImGui::Text(("Minute: " + to_str(kernel.CALENDAR_MINUTE, 0)).c_str());
		ImGui::Text(("Zoom:   " + to_str(camera_zoom_sensitivity, 1)).c_str());
		ImGui::SameLine();
		ImGui::SetCursorPosX(halfPos);
		ImGui::Text(("Orbit:  " + to_str(camera_orbit_sensitivity, 1)).c_str());
		ImGui::PopItemWidth();
	}

	pathtracer.f_guiUpdate(availableWidth, spacing, itemWidth, halfWidth, thirdWidth, halfPos);

	if (ImGui::CollapsingHeader("Performance")) {
		ImGui::SeparatorText("Average Stats");
		{
			ImGui::PushItemWidth(thirdWidth);
			const dvec1 percent = round((gpu_time_aggregate / chrono::duration<double>(current_time - start_time).count()) * 100.0);
			ImGui::Text(("~GPU[" + to_str(percent, 0) + "]%%").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos);
			ImGui::Text(("~CPU[" + to_str(100.0 - percent, 0) + "]%%").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos * 2.0f);
			ImGui::Text(("Fps: " + to_str(ul_to_d(runframe) / chrono::duration<double>(current_time - start_time).count(), 0)).c_str());
			ImGui::PopItemWidth();
		}
		ImGui::SeparatorText("Stats");
		{
			ImGui::PushItemWidth(thirdWidth);
			const dvec1 percent = round((gpu_time / frame_time) * 100.0);
			ImGui::Text(("~GPU[" + to_str(percent, 0) + "]%%").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos);
			ImGui::Text(("~CPU[" + to_str(100.0 - percent, 0) + "]%%").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos * 2.0f);
			ImGui::Text(("Fps: " + to_str(frame_count, 0)).c_str());
			ImGui::PopItemWidth();
		}
		ImGui::SeparatorText("Metrics");
		END_TIMER("GUI Loop");
		{
			const dvec1 probe = TIMER_MARK("Probe BVH");
			const dvec1 particle = TIMER_MARK("Particle BVH");
			const dvec1 scatter = TIMER_MARK("Scatter");
			const dvec1 gather = TIMER_MARK("Gather");
			const dvec1 particle_update = TIMER_MARK("Particle Update");
			const dvec1 gui = TIMER_MARK("GUI Loop");
			const dvec1 cpu_time = TIMER_MARK("CPU");
			const dvec1 gpu_time = TIMER_MARK("GPU");
			const dvec1 delta_time = TIMER_MARK("Delta");
			const dvec1 transfer = TIMER_MARK("Transfer");
			ImGui::PushItemWidth(thirdWidth);
			ImGui::Text("Delta");
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos);
			ImGui::Text((to_str(d_to_f(delta_time * 1000.0), 2) + " ms").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos * 2.0f);
			ImGui::Text((to_str(d_to_f(delta_time / frame_count * 1000.0), 2) + " ms").c_str());

			ImGui::Text("CPU");
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos);
			ImGui::Text((to_str(d_to_f(cpu_time * 1000.0), 2) + " ms").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos * 2.0f);
			ImGui::Text((to_str(d_to_f(cpu_time / frame_count * 1000.0), 2) + " ms").c_str());

			ImGui::Text("GPU");
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos);
			ImGui::Text((to_str(d_to_f(gpu_time * 1000.0), 2) + " ms").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos * 2.0f);
			ImGui::Text((to_str(d_to_f(gpu_time / frame_count * 1000.0), 2) + " ms").c_str());

			ImGui::Text("CPU -> GPU");
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos);
			ImGui::Text((to_str(d_to_f(transfer * 1000.0), 2) + " ms").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos * 2.0f);
			ImGui::Text((to_str(d_to_f(transfer / frame_count * 1000.0), 2) + " ms").c_str());

			ImGui::Text("Probe BVH");
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos);
			ImGui::Text((to_str(d_to_f(probe * 1000.0), 2) + " ms").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos * 2.0f);
			ImGui::Text((to_str(d_to_f(probe / frame_count * 1000.0), 2) + " ms").c_str());

			ImGui::Text("Particle BVH");
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos);
			ImGui::Text((to_str(d_to_f(particle * 1000.0), 2) + " ms").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos * 2.0f);
			ImGui::Text((to_str(d_to_f(particle / frame_count * 1000.0), 2) + " ms").c_str());

			ImGui::Text("Scatter");
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos);
			ImGui::Text((to_str(d_to_f(scatter * 1000.0), 2) + " ms").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos * 2.0f);
			ImGui::Text((to_str(d_to_f(scatter / frame_count * 1000.0), 2) + " ms").c_str());

			ImGui::Text("Gather");
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos);
			ImGui::Text((to_str(d_to_f(gather * 1000.0), 2) + " ms").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos * 2.0f);
			ImGui::Text((to_str(d_to_f(gather / frame_count * 1000.0), 2) + " ms").c_str());

			ImGui::Text("Particle Update");
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos);
			ImGui::Text((to_str(d_to_f(particle_update * 1000.0), 2) + " ms").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos * 2.0f);
			ImGui::Text((to_str(d_to_f(particle_update / frame_count * 1000.0), 2) + " ms").c_str());

			ImGui::Text("GUI Loop");
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos);
			ImGui::Text((to_str(d_to_f(gui * 1000.0), 2) + " ms").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos * 2.0f);
			ImGui::Text((to_str(d_to_f(gui / frame_count * 1000.0), 2) + " ms").c_str());

			ImGui::Text("Other");
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos);
			ImGui::Text((to_str(d_to_f((cpu_time - (probe + particle + scatter + gather + particle_update + gui + transfer)) * 1000.0), 2) + " ms").c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(thirdPos * 2.0f);
			ImGui::Text((to_str(d_to_f((cpu_time - (probe + particle + scatter + gather + particle_update + gui + transfer)) / frame_count * 1000.0), 2) + " ms").c_str());
			ImGui::PopItemWidth();
		}
	}

	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::f_frameUpdate() {
	gpu_time_aggregate += gpu_delta;
	frame_timer += delta_time;
	gpu_timer += gpu_delta;
	frame_counter++;
	runframe++;
	TIMER("GPU") = gpu_delta;
	TIMER("CPU") = abs(delta_time - gpu_delta);
	TIMER("Delta") = gpu_delta + abs(delta_time - gpu_delta);

	UPDATE_TIMER_MARK("Probe BVH");
	UPDATE_TIMER_MARK("Particle BVH");
	UPDATE_TIMER_MARK("Scatter");
	UPDATE_TIMER_MARK("Gather");
	UPDATE_TIMER_MARK("Particle Update");
	UPDATE_TIMER_MARK("GUI Loop");
	UPDATE_TIMER_MARK("GPU");
	UPDATE_TIMER_MARK("CPU");
	UPDATE_TIMER_MARK("Delta");
	UPDATE_TIMER_MARK("Transfer");
	if (window_time > 1.0) {
		RESET_TIMER_MARK("Probe BVH");
		RESET_TIMER_MARK("Particle BVH");
		RESET_TIMER_MARK("Scatter");
		RESET_TIMER_MARK("Gather");
		RESET_TIMER_MARK("Particle Update");
		RESET_TIMER_MARK("GUI Loop");
		RESET_TIMER_MARK("GPU");
		RESET_TIMER_MARK("CPU");
		RESET_TIMER_MARK("Delta");
		RESET_TIMER_MARK("Transfer");

		frame_count = frame_counter;
		frame_time = frame_timer;
		gpu_time = gpu_timer;

		frame_counter = 0;
		frame_timer = 0.0;
		gpu_timer = 0.0;
		window_time -= 1.0;
	}
}

void Renderer::f_inputLoop() {
	if (lock_view) {
		const dmat4 rotmat = glm::rotate(dmat4(1.0), -glm::radians(kernel.EARTH_TILT), dvec3(0, 0, 1));
		const dvec3 tilt = glm::normalize(dvec3(rotmat * dvec4(0,1,0,1)));
		world_rot = glm::rotate(world_rot, glm::radians(delta_time * kernel.TIME_SCALE * 0.01 * 360.0), tilt);
	}
	camera = world_rot * dquat(camera_transform.euler_rotation);
	for (uint i = 0; i < 6; i++) {
		dvec3* input = &input_lerps[i];
		if (input->y < 1.0) {
			input->y += delta_time / input_lerp;
			if (input->y > 1.0)
				input->y = 1.0;
			input->x = lerp(input->x, input->z, input->y);
		}
		if (i == 0) {
			camera_transform.orbit(dvec3(0), dvec2(-1, 0) * input->x * camera_orbit_sensitivity * 10.0 * delta_time);
		}
		if (i == 1) {
			camera_transform.orbit(dvec3(0), dvec2(0, -1) * input->x * camera_orbit_sensitivity * 10.0 * delta_time);
		}
		if (i == 2) {
			camera_transform.orbit(dvec3(0), dvec2(1, 0) * input->x * camera_orbit_sensitivity * 10.0 * delta_time);
		}
		if (i == 3) {
			camera_transform.orbit(dvec3(0), dvec2(0, 1) * input->x * camera_orbit_sensitivity * 10.0 * delta_time);
		}
		if (i == 4) {
			zoom -= 1.0 * input->x * camera_zoom_sensitivity * delta_time;
		}
		if (i == 5) {
			zoom += 1.0 * input->x * camera_zoom_sensitivity * delta_time;
		}
	}
	if (!lock_view) {
		//const dvec3 eulerAngles = glm::eulerAngles(camera);
		//dvec1 pitch = eulerAngles.x;
		//const dvec1 yaw = eulerAngles.y;
		//const dvec1 pitchLimit = glm::radians(89.0f);
		//pitch = glm::clamp(pitch, -pitchLimit, pitchLimit);
		//camera = glm::angleAxis(pitch, dvec3(1, 0, 0)) *  glm::angleAxis(yaw, dvec3(0, 1, 0));
	}
}

void Renderer::f_timings() {
	current_time = chrono::high_resolution_clock::now();
	delta_time = chrono::duration<double>(current_time - last_time).count();
	last_time = current_time;
	window_time += delta_time;
}

void Renderer::f_displayLoop() {
	while (!glfwWindowShouldClose(window)) {

		f_timings();
		f_inputLoop();
		f_tickUpdate();
		const auto start = chrono::high_resolution_clock::now();
		pathtracer.f_render();
		gpu_delta = chrono::duration<double>(chrono::high_resolution_clock::now() - start).count();

		f_frameUpdate();
		f_guiLoop();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void Renderer::f_resize() {
	display_aspect_ratio = u_to_d(display_resolution.x) / u_to_d(display_resolution.y);
	render_resolution = d_to_u(u_to_d(display_resolution) * f_to_d(render_scale));
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
		instance->pathtracer.f_recompile();
	}
	//if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
	//	glfwSetWindowShouldClose(window, GLFW_TRUE);
	//}
	if (key == GLFW_KEY_L && action == GLFW_PRESS) {
		instance->lock_view = !instance->lock_view;
	}
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		instance->kernel.TIME_SCALE /= 0.9;
	}
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		instance->kernel.TIME_SCALE *= 0.9;
	}
	if (key == GLFW_KEY_PERIOD && action == GLFW_PRESS) {
		instance->camera_orbit_sensitivity /= 0.9;
		instance->camera_zoom_sensitivity /= 0.9;
	}
	if (key == GLFW_KEY_COMMA && action == GLFW_PRESS) {
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