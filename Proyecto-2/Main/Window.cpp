#include "Window.hpp"

#undef NL
#undef FILE

#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"

Renderer::Renderer() {
	window = nullptr;

	SESSION_SET("PARTICLE_DISPLAY_RADIUS", 0.01, dvec1);
	SESSION_SET("PARTICLE_RADIUS", 0.01, dvec1);
	SESSION_SET("PARTICLE_COUNT", 1024, uint64);

	SESSION_SET("RENDER_SCALE", 0.5, dvec1);
	SESSION_SET("TIME_SCALE", 0.05, dvec1);

	SESSION_SET("GRID_SIZE_X", 16, uint64);
	SESSION_SET("GRID_SIZE_Y", 16, uint64);
	SESSION_SET("GRID_SIZE_Z", 16, uint64);
	SESSION_SET("CELL_SIZE", 1.0 / max(max(SESSION_GET("GRID_SIZE_X", uint64), SESSION_GET("GRID_SIZE_Y", uint64)), SESSION_GET("GRID_SIZE_Z", uint64)), dvec1);

	camera_transform = Transform(dvec3(0, 0, 4), dvec3(0));
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

	render_resolution = d_to_u(u_to_d(display_resolution) * SESSION_GET("RENDER_SCALE", dvec1));
	render_aspect_ratio = u_to_d(render_resolution.x) / u_to_d(render_resolution.y);

	recompile = false;

	camera_zoom_sensitivity = 0.1;
	camera_orbit_sensitivity = 5.0;
	inputs = vector(348, false);

	sim_time_aggregate = 0.0;
	flip = Flip();

	current_time = 0.0;
	window_time = 0.0;
	delta_time = FPS_60;
	sim_delta = FPS_60 / 2.0;
	last_time = 0.0;

	start_sim = false;
	render_grid = new bool(true);
	render_particles = new bool(true);

	render_grid_surface = new bool(false);
	render_grid_density = new bool(false);

	render_grid_opacity = new vec1(1.0f);
	render_grid_density_mul = new vec1(1.0f);
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
	glfwSwapInterval(false); // V-Sync
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
	io.Fonts->AddFontFromFileTTF("./Resources/RobotoMono-Medium.ttf", 16.0f);
	io.FontGlobalScale = 2.5f;

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
	flip.init();
}

void Renderer::f_tickUpdate() {
	if (start_sim) {
		const dvec1 start = glfwGetTime();
		const dvec1 delta = delta_time * SESSION_GET("TIME_SCALE", dvec1);
		flip.simulate(delta);
		sim_delta = glfwGetTime() - start;
	}
	else {
		sim_delta = 0.0;
	}

	glDeleteBuffers(1, &buffers["particles"]);
	vector<GPU_Particle> gpu_point_cloud = flip.gpuParticles();
	buffers["particles"] = ssboBinding(1, ul_to_u(gpu_point_cloud.size() * sizeof(GPU_Particle)), gpu_point_cloud.data());
	
	glDeleteBuffers(1, &buffers["cells"]);
	vector<GPU_Cell> gpu_grid = flip.gpuGrid();
	buffers["cells"] = ssboBinding(2, ul_to_u(gpu_grid.size() * sizeof(GPU_Cell)), gpu_grid.data());
}

void Renderer::guiLoop() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::Begin("Info");

	{
		ImGui::Text(("Avg. Frame Delta: " + to_str((current_time / ul_to_d(runframe) * 1000.0), 3) + "ms").c_str());
		ImGui::Text(("Avg. Sim   Delta: " + to_str((sim_time_aggregate / ul_to_d(runframe) * 1000.0), 3) + "ms").c_str());

		const dvec1 percent = round((sim_time_aggregate / current_time) * 100.0);
		ImGui::Text(("Avg. ~GPU[" + to_str(100.0 - percent, 0) + "]%%").c_str());
		ImGui::Text(("Avg. ~CPU[" + to_str(percent, 0) + "]%%").c_str());

		ImGui::Text(("Avg. Fps: " + to_str(ul_to_d(runframe) / current_time, 0)).c_str());
	}
	ImGui::Separator();
	{
		const dvec1 percent = round((sim_time / frame_time) * 100.0);
		ImGui::Text(("~GPU[" + to_str(100.0 - percent, 0) + "]%%").c_str());
		ImGui::Text(("~CPU[" + to_str(percent, 0) + "]%%").c_str());

		ImGui::Text(("Fps: " + to_str(frame_count, 0)).c_str());
	}
	ImGui::Separator();

	ImGui::Checkbox("Render Grid", render_grid);
	if (*render_grid) {
		ImGui::Separator();
		ImGui::Checkbox("Render Surface", render_grid_surface);
		ImGui::Checkbox("Render Density", render_grid_density);
		if (*render_grid_density) {
			ImGui::SliderFloat("Density Mul", render_grid_density_mul, 0.05f, 2.0f, "%.4f");
		}
		else {
			ImGui::SliderFloat("Opacity Mul", render_grid_opacity, 0.05f, 2.0f, "%.4f");
		}
		ImGui::Separator();
	}
	ImGui::Checkbox("Render Particles", render_particles);

	if (not start_sim) {
		if (ImGui::Button("Start")) {
			start_sim = true;
		}
	}

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
	const uvec3 compute_layout = uvec3(
		d_to_u(ceil(u_to_d(render_resolution.x) / 32.0)),
		d_to_u(ceil(u_to_d(render_resolution.y) / 32.0)),
		1
	);

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

		glUniform3ui (glGetUniformLocation(compute_program, "grid_size"), ul_to_u(SESSION_GET("GRID_SIZE_X", uint64)), ul_to_u(SESSION_GET("GRID_SIZE_Y", uint64)), ul_to_u(SESSION_GET("GRID_SIZE_Z", uint64)));
		glUniform1f  (glGetUniformLocation(compute_program, "cell_size"), d_to_f(SESSION_GET("CELL_SIZE", dvec1)));
		glUniform1f  (glGetUniformLocation(compute_program, "sphere_radius"), d_to_f(SESSION_GET("PARTICLE_RADIUS", dvec1)));
		glUniform1f  (glGetUniformLocation(compute_program, "sphere_display_radius"), d_to_f(SESSION_GET("PARTICLE_DISPLAY_RADIUS", dvec1)));

		glUniform1ui (glGetUniformLocation(compute_program, "render_grid"), *render_grid);
		glUniform1ui (glGetUniformLocation(compute_program, "render_particles"), *render_particles);

		glUniform1ui (glGetUniformLocation(compute_program, "render_grid_surface"), *render_grid_surface);
		glUniform1ui (glGetUniformLocation(compute_program, "render_grid_density"), *render_grid_density);

		glUniform1f  (glGetUniformLocation(compute_program, "render_grid_opacity"), *render_grid_opacity);
		glUniform1f  (glGetUniformLocation(compute_program, "render_grid_density_mul"), *render_grid_density_mul);

		glBindImageTexture(0, buffers["raw"], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		glDispatchCompute(compute_layout.x, compute_layout.y, compute_layout.z);
		
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		GLuint display_program = buffers["display"];

		glUseProgram(display_program);
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform1f (glGetUniformLocation(display_program, "display_aspect_ratio"), d_to_f(display_aspect_ratio));
		glUniform1f (glGetUniformLocation(display_program, "render_aspect_ratio") , d_to_f(render_aspect_ratio));
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