#include "Window.hpp"

#undef NL
#undef FILE

#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"

Renderer::Renderer() {
	window = nullptr;

	camera_transform = Transform(dvec3(0, 0, 37.5), dvec3(0));
	camera_transform.orbit(dvec3(0), dvec3(-15, 15, 0));

	runframe = 0;

	frame_counter = 0;
	frame_timer = 0;
	sim_timer = 0;

	frame_count = 0;
	frame_time = 0;
	sim_time = 0;

	display_resolution = uvec2(3840U, 2160U);
	display_aspect_ratio = u_to_d(display_resolution.x) / u_to_d(display_resolution.y);

	recompile = false;

	camera_zoom_sensitivity = 2.0;
	camera_orbit_sensitivity = 5.0;
	inputs = vector(348, false);

	texture_size = 0;

	sim_time_aggregate = 0.0;
	kernel = Kernel();

	current_time = 0.0;
	window_time = 0.0;
	delta_time = FPS_60;
	sim_delta = FPS_60 / 2.0;
	last_time = 0.0;

	run_sim = false;

	TIME_SCALE       = 0.1f;
	RENDER_SCALE     = 0.25f;
	PARTICLE_RADIUS  = 0.065f;
	PARTICLE_COUNT   = 2048;
	MAX_PARTICLES    = 4096 * 4;
	LAYER_COUNT      = 1;
	PARTICLE_DISPLAY = 1.0f;
	MAX_OCTREE_DEPTH = 7;

	POLE_BIAS = 0.95f;
	POLE_BIAS_POWER = 2.5f;
	POLE_GEOLOCATION = vec2(23.1510, 93.0422);


	render_resolution = d_to_u(u_to_d(display_resolution) * f_to_d(RENDER_SCALE));
	render_aspect_ratio = u_to_d(render_resolution.x) / u_to_d(render_resolution.y);

	compute_layout = uvec3(
		d_to_u(ceil(u_to_d(render_resolution.x) / 32.0)),
		d_to_u(ceil(u_to_d(render_resolution.y) / 32.0)),
		1
	);

	render_planet = true;
	render_octree = false;
	{
		render_octree_hue = false;
		render_octree_debug = false;
		render_octree_debug_index = 0;
	}
	render_particles = true;
	{
		render_particle_color_mode = 0;
	}
	render_planet_texture = 0;
}

Renderer::~Renderer() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	glDeleteProgram(buffers["compute"]);
	glDeleteProgram(buffers["display"]);
	glDeleteBuffers(1, &buffers["particles"]);
	glDeleteBuffers(1, &buffers["bvh_nodes"]);
	glDeleteBuffers(1, &buffers["textures"]);
	glDeleteBuffers(1, &buffers["texture_data"]);
	glDeleteTextures(1, &buffers["raw"]);
}

void Renderer::init() {
	initGlfw();
	initImGui();
	systemInfo();

	f_pipeline();
	displayLoop();
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
	glViewport(0, 0, display_resolution.x , display_resolution.y);
	
	const GLfloat vertices[16] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		-1.0f,  1.0f, 0.0f, 1.0f,
		 1.0f,  1.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 1.0f, 0.0f,
	};
	const GLuint indices[6] = {
		0, 1, 2,
		2, 3, 0
	};

	// Display Quad
	GLuint VAO, VBO, EBO;
	glCreateVertexArrays(1, &VAO);
	glCreateBuffers(1, &VBO);
	glCreateBuffers(1, &EBO);

	glNamedBufferData(VBO, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glNamedBufferData(EBO, sizeof(indices), indices, GL_STATIC_DRAW);

	glEnableVertexArrayAttrib (VAO, 0);
	glVertexArrayAttribBinding(VAO, 0, 0);
	glVertexArrayAttribFormat (VAO, 0, 2, GL_FLOAT, GL_FALSE, 0);

	glEnableVertexArrayAttrib (VAO, 1);
	glVertexArrayAttribBinding(VAO, 1, 0);
	glVertexArrayAttribFormat (VAO, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat));

	glVertexArrayVertexBuffer (VAO, 0, VBO, 0, 4 * sizeof(GLfloat));
	glVertexArrayElementBuffer(VAO, EBO);

	buffers["compute"] = computeShaderProgram("Render");
	buffers["display"] = fragmentShaderProgram("Display");
	FLUSH;

	// Compute Output
	buffers["raw"] = renderLayer(render_resolution);

	glBindVertexArray(VAO);
	initKernel();
	compute_layout = uvec3(
		d_to_u(ceil(u_to_d(render_resolution.x) / 32.0)),
		d_to_u(ceil(u_to_d(render_resolution.y) / 32.0)),
		1
	);

	vector<uint> texture_data;
	vector<string> texture_names = { "Albedo", "Sea Surface Temperature", "Land Surface Temperature" };
	for (const string& tex : texture_names) {
		Texture texture = Texture::fromFile("./Resources/Nasa Earth Data/" + tex + ".png");
		textures.push_back(GPU_Texture(ul_to_u(texture_data.size()), texture.resolution.x, texture.resolution.y, 0));
		auto data = texture.toRgba8Texture();
		texture_data.insert(texture_data.end(), data.begin(), data.end());
	}
	texture_size = ul_to_u(texture_data.size());
	buffers["textures"] = ssboBinding(3, ul_to_u(textures.size() * sizeof(GPU_Texture)), textures.data());
	buffers["texture_data"] = ssboBinding(4, ul_to_u(texture_data.size() * sizeof(uint)), texture_data.data());
	vector<GPU_Particle> gpu_point_cloud = kernel.gpuParticles();
	buffers["particles"] = ssboBinding(1, ul_to_u(gpu_point_cloud.size() * sizeof(GPU_Particle)), gpu_point_cloud.data());
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

	//glDeleteBuffers(1, &buffers["particles"]);
	//vector<GPU_Particle> gpu_point_cloud = kernel.gpuParticles();
	//buffers["particles"] = ssboBinding(1, ul_to_u(gpu_point_cloud.size() * sizeof(GPU_Particle)), gpu_point_cloud.data());
}

void Renderer::initKernel() {
	kernel.init(PARTICLE_RADIUS, PARTICLE_COUNT, LAYER_COUNT, MAX_OCTREE_DEPTH, POLE_BIAS, POLE_BIAS_POWER, POLE_GEOLOCATION);

	glDeleteBuffers(1, &buffers["bvh_nodes"]);
	buffers["bvh_nodes"] = ssboBinding(2, ul_to_u(kernel.bvh_nodes.size() * sizeof(GPU_Bvh)), kernel.bvh_nodes.data());

	glDeleteBuffers(1, &buffers["particles"]);
	vector<GPU_Particle> gpu_point_cloud = kernel.gpuParticles();
	buffers["particles"] = ssboBinding(1, ul_to_u(gpu_point_cloud.size() * sizeof(GPU_Particle)), gpu_point_cloud.data());
}

void Renderer::guiLoop() {
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
		resize();
	}
	ImGui::Checkbox("Render Planet", &render_planet);
	if (render_planet) {
		const char* items_b[] = { "Albedo", "Sea Surface Temperature", "Land Surface Temperature" };
		ImGui::Combo("Planet Texture", &render_planet_texture, items_b, IM_ARRAYSIZE(items_b));
	}
	ImGui::Checkbox("Render Octree", &render_octree);
	if (render_octree) {
		ImGui::Checkbox("Hue", &render_octree_hue);
		ImGui::Checkbox("Debug", &render_octree_debug);
		if (render_octree_debug) {
			ImGui::SliderInt("Index", &render_octree_debug_index, 0, ul_to_i(kernel.bvh_nodes.size()));
		}
	}

	ImGui::SeparatorText("Play / Pause");
	if (run_sim) {
		if (ImGui::Button("Stop")) {
			run_sim = false;
		}
		if (ImGui::Button("Restart")) {
			run_sim = false;
			initKernel();
		}
	}
	else {
		if (ImGui::Button("Start")) {
			run_sim = true;
		}
		ImGui::SeparatorText("Init Settings");
		if (ImGui::SliderFloat("Particle Radius", &PARTICLE_RADIUS, 0.001f, 1.0f, "%.5f")) {
			initKernel();
		}

		if (ImGui::SliderFloat("Display Mult", &PARTICLE_DISPLAY, 0.05f, 5.0f, "%.3f")) {
			initKernel();
		}

		if (ImGui::SliderInt("Particle Count", &PARTICLE_COUNT, 128, MAX_PARTICLES)) {
			initKernel();
		}

		if (ImGui::SliderInt("Max Octree Depth", &MAX_OCTREE_DEPTH, 0, 10)) {
			initKernel();
		}

		if (ImGui::SliderInt("Layer Count", &LAYER_COUNT, 1, 16)) {
			initKernel();
		}

		if (ImGui::SliderFloat("Pole Bias", &POLE_BIAS, 0.0f, 1.0f, "%.5f")) {
			initKernel();
		}

		if (ImGui::SliderFloat("Pole Power", &POLE_BIAS_POWER, 1.0f, 3.0f)) {
			initKernel();
		}

		if (ImGui::SliderFloat("Latitude", &POLE_GEOLOCATION.x, -90.0f, 90.0f), "%.4f") {
			initKernel();
		}
		if (ImGui::SliderFloat("Longitude", &POLE_GEOLOCATION.y, -180.0f, 180.0f, "%.4f")) {
			initKernel();
		}
	}
	ImGui::SeparatorText("Particle Settings");
	ImGui::Checkbox("Render Particles", &render_particles);
	if (render_particles) {
		const char* items_b[] = { "Temperature", "Velocity" };
		ImGui::Combo("Particle Color Mode", &render_particle_color_mode, items_b, IM_ARRAYSIZE(items_b));
	}
	ImGui::SeparatorText("Theoretical Performance Stats");
	//ImGui::Text(string("RAM: " + to_string((PARTICLE_COUNT * sizeof(CPU_Particle)) / 1024) + "mb ").c_str());
	ImGui::Text(string("Octree VRAM: " + to_string((kernel.bvh_nodes.size() * sizeof(GPU_Bvh)) / 1024) + "mb ").c_str());
	ImGui::Text(string("Texture VRAM: " + to_string((texture_size * sizeof(uint)) / 1024) + "mb ").c_str());
	ImGui::Text(string("Particle VRAM: " + to_string((PARTICLE_COUNT * sizeof(GPU_Particle)) / 1024) + "mb ").c_str());

	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::gameLoop() {
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

void Renderer::displayLoop() {
	while (!glfwWindowShouldClose(window)) {
		current_time = glfwGetTime();
		delta_time = current_time - last_time;
		last_time = current_time;
		window_time += delta_time;

		gameLoop();
		const mat4 matrix = d_to_f(glm::yawPitchRoll(camera_transform.euler_rotation.y * DEG_RAD, camera_transform.euler_rotation.x * DEG_RAD, camera_transform.euler_rotation.z * DEG_RAD));
		const vec3 y_vector = matrix[1];
		const vec3 z_vector = -matrix[2];

		const vec1 focal_length = 0.05f;
		const vec1 sensor_size  = 0.036f;

		const vec3 projection_center = d_to_f(camera_transform.position) + focal_length * z_vector;
		const vec3 projection_u = normalize(cross(z_vector, y_vector)) * sensor_size;
		const vec3 projection_v = normalize(cross(projection_u, z_vector)) * sensor_size;

		f_tickUpdate();

		GLuint compute_program = buffers["compute"];

		glUseProgram(compute_program);
		glUniform1ui (glGetUniformLocation(compute_program, "frame_count"), ul_to_u(runframe));
		glUniform1f  (glGetUniformLocation(compute_program, "aspect_ratio"), d_to_f(render_aspect_ratio));
		glUniform1f  (glGetUniformLocation(compute_program, "current_time"), d_to_f(current_time));
		glUniform2ui (glGetUniformLocation(compute_program, "resolution"), render_resolution.x, render_resolution.y);

		glUniform3fv (glGetUniformLocation(compute_program, "camera_pos"), 1, value_ptr(d_to_f(camera_transform.position)));
		glUniform3fv (glGetUniformLocation(compute_program, "camera_p_uv"),1, value_ptr(projection_center));
		glUniform3fv (glGetUniformLocation(compute_program, "camera_p_u"), 1, value_ptr(projection_u));
		glUniform3fv (glGetUniformLocation(compute_program, "camera_p_v"), 1, value_ptr(projection_v));

		glUniform3fv (glGetUniformLocation(compute_program, "root_bvh.p_min"), 1, value_ptr(kernel.root_node.p_min));
		glUniform1ui (glGetUniformLocation(compute_program, "root_bvh.particle_start"), kernel.root_node.particle_start);
		glUniform3fv (glGetUniformLocation(compute_program, "root_bvh.p_max"), 1, value_ptr(kernel.root_node.p_max));
		glUniform1ui (glGetUniformLocation(compute_program, "root_bvh.particle_end"), kernel.root_node.particle_end);
		glUniform4iv (glGetUniformLocation(compute_program, "root_bvh.pointers_a"), 1, value_ptr(kernel.root_node.pointers_a));
		glUniform4iv (glGetUniformLocation(compute_program, "root_bvh.pointers_b"), 1, value_ptr(kernel.root_node.pointers_b));

		glUniform1f  (glGetUniformLocation(compute_program, "sphere_radius"), PARTICLE_RADIUS);
		glUniform1f  (glGetUniformLocation(compute_program, "sphere_display_radius"), PARTICLE_DISPLAY * PARTICLE_RADIUS);

		glUniform1ui (glGetUniformLocation(compute_program, "render_planet"), render_planet);
		glUniform1ui (glGetUniformLocation(compute_program, "render_octree"), render_octree);
		{
			glUniform1ui (glGetUniformLocation(compute_program, "render_octree_hue"), render_octree_hue);
			glUniform1ui (glGetUniformLocation(compute_program, "render_octree_debug"), render_octree_debug);
			glUniform1i  (glGetUniformLocation(compute_program, "render_octree_debug_index"), render_octree_debug_index);
		}
		glUniform1ui (glGetUniformLocation(compute_program, "render_particles"), render_particles);
		{
			glUniform1i(glGetUniformLocation(compute_program, "render_particle_color_mode"), render_particle_color_mode);
		}
		glUniform1i(glGetUniformLocation(compute_program, "render_planet_texture"), render_planet_texture);

		glBindImageTexture(0, buffers["raw"], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

		glDispatchCompute(compute_layout.x, compute_layout.y, compute_layout.z);
		
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		GLuint display_program = buffers["display"];

		glUseProgram(display_program);
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform1f  (glGetUniformLocation(display_program, "display_aspect_ratio"), d_to_f(display_aspect_ratio));
		glUniform1f  (glGetUniformLocation(display_program, "render_aspect_ratio") , d_to_f(render_aspect_ratio));
		glUniform2uiv(glGetUniformLocation(display_program, "display_resolution"), 1, value_ptr(display_resolution));
		glUniform2uiv(glGetUniformLocation(display_program, "render_resolution") , 1, value_ptr(render_resolution));
		bindRenderLayer(display_program, 0, buffers["raw"], "raw_render_layer");
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		if (recompile) {
			glDeleteProgram(buffers["compute"]);
			glDeleteProgram(buffers["display"]);
			buffers["compute"] = computeShaderProgram("Render");
			buffers["display"] = fragmentShaderProgram("Display");
			recompile = false;
		}

		if (window_time > 1.0) {
			frame_count = frame_counter;
			frame_time = frame_timer;
			sim_time = sim_timer;

			frame_counter = 0;
			frame_timer = 0.0;
			sim_timer = 0.0;
			window_time -= 1.0;
		}

		guiLoop();
		sim_time_aggregate += sim_delta;
		frame_timer += delta_time;
		sim_timer += sim_delta;
		frame_counter++;
		runframe++;

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void Renderer::resize() {
	display_aspect_ratio = u_to_d(display_resolution.x) / u_to_d(display_resolution.y);
	render_resolution = d_to_u(u_to_d(display_resolution) * f_to_d(RENDER_SCALE));
	render_aspect_ratio = u_to_d(render_resolution.x) / u_to_d(render_resolution.y);
	glDeleteTextures(1, &buffers["raw"]);
	buffers["raw"] = renderLayer(render_resolution);
	compute_layout = uvec3(
		d_to_u(ceil(u_to_d(render_resolution.x) / 32.0)),
		d_to_u(ceil(u_to_d(render_resolution.y) / 32.0)),
		1
	);
}

void Renderer::framebufferSize(GLFWwindow* window, int width, int height) {
	Renderer* instance = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	glViewport(0, 0, width, height);
	instance->display_resolution.x = width;
	instance->display_resolution.y = height;
	instance->resize();
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
		instance->recompile = true;
	}
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	if (action == GLFW_PRESS) {
		instance->inputs[key] = true;
	}
	else if (action == GLFW_RELEASE) {
		instance->inputs[key] = false;
	}
}