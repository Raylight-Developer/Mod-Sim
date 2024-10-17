#include "Window.hpp"

#undef NL
#undef FILE

#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"

Renderer::Renderer() {
	window = nullptr;

	SESSION_SET("PARTICLE_DISPLAY_RADIUS", 0.01, dvec1);
	SESSION_SET("PARTICLE_RADIUS", 0.01, dvec1);
	SESSION_SET("PARTICLE_COUNT", 128, uint64);

	SESSION_SET("RENDER_SCALE", 0.5, dvec1);

	SESSION_SET("GRID_SIZE_X", 16, uint64);
	SESSION_SET("GRID_SIZE_Y", 16, uint64);
	SESSION_SET("GRID_SIZE_Z", 16, uint64);
	SESSION_SET("CELL_SIZE", 1.0 / max(max(SESSION_GET("GRID_SIZE_X", uint64), SESSION_GET("GRID_SIZE_Y", uint64)), SESSION_GET("GRID_SIZE_Z", uint64)), dvec1);

	camera_transform = Transform(dvec3(0, 5, 0), dvec3(-90.0, 0, 0.0));

	frame_counter = 0;
	frame_count = 0;
	runframe = 0;

	display_resolution = uvec2(3840U, 2160U);
	display_aspect_ratio = u_to_d(display_resolution.x) / u_to_d(display_resolution.y);

	render_resolution = d_to_u(u_to_d(display_resolution) * SESSION_GET("RENDER_SCALE", dvec1));
	render_aspect_ratio = u_to_d(render_resolution.x) / u_to_d(render_resolution.y);

	recompile = false;

	camera_zoom_sensitivity = 1.0;
	camera_orbit_sensitivity = 2.5;
	keys = vector(348, false);
	current_mouse = dvec2(display_resolution) / 2.0;
	last_mouse = dvec2(display_resolution) / 2.0;

	sim_deltas = 0.0;

	current_time = 0.0;
	window_time = 0.0;
	frame_time = FPS_60;
	sim_delta = FPS_60 / 2.0;
	last_time = 0.0;
}

void Renderer::init() {
	initGlfw();
	initImGui();
	systemInfo();

	f_pipeline();
	displayLoop();
}

void Renderer::quit() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(0);
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
	last_mouse = glm::dvec2(display_resolution) / 2.0;

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
	glfwSetCursorPosCallback(window, cursorPos);
	glfwSetScrollCallback(window, scroll);
	glfwSetKeyCallback(window, key);
}

void Renderer::initImGui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.FontGlobalScale = 1.75f;
	io.IniFilename = nullptr;

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
	cpu_point_cloud = vector(SESSION_GET("PARTICLE_COUNT", uint64), CPU_Particle());
	cpu_grid = vector(SESSION_GET("GRID_SIZE_X", uint64), vector(SESSION_GET("GRID_SIZE_Y", uint64), vector(SESSION_GET("GRID_SIZE_Z", uint64), CPU_Cell())));
	initialize(cpu_point_cloud);
	initialize(cpu_grid);
}

void Renderer::f_tickUpdate() {
	const dvec1 start = glfwGetTime();
	if (runframe > 10) {
		simulate(cpu_point_cloud, 0.0005);
		simulate(cpu_grid, 0.0005);
	}

	sim_delta = glfwGetTime() - start;

	glDeleteBuffers(1, &buffers["particles"]);
	vector<GPU_Particle> gpu_point_cloud;
	for (const CPU_Particle& particle : cpu_point_cloud) {
		gpu_point_cloud.push_back(GPU_Particle(particle));
	}
	buffers["particles"] = ssboBinding(1, ul_to_u(gpu_point_cloud.size() * sizeof(GPU_Particle)), gpu_point_cloud.data());

	glDeleteBuffers(1, &buffers["cells"]);
	const ulvec3 GRID_SIZE = ulvec3(SESSION_GET("GRID_SIZE_X", uint64),SESSION_GET("GRID_SIZE_Y", uint64),SESSION_GET("GRID_SIZE_Z", uint64));
	vector<GPU_Cell> gpu_grid(GRID_SIZE.x * GRID_SIZE.y * GRID_SIZE.z);
	for (uint64 x = 0; x < GRID_SIZE.x; ++x) {
		for (uint64 y = 0; y < GRID_SIZE.y; ++y) {
			for (uint64 z = 0; z < GRID_SIZE.z; ++z) {
				uint64 index = x * (GRID_SIZE.y * GRID_SIZE.z) + y * GRID_SIZE.z + z;
				gpu_grid[index] = GPU_Cell(cpu_grid[x][y][z]);
			}
		}
	}
	buffers["cells"] = ssboBinding(2, ul_to_u(gpu_grid.size() * sizeof(GPU_Cell)), gpu_grid.data());
}

void Renderer::guiLoop() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::Begin("Info");
	ImGui::Text(("Avg. Frame Delta: " + to_str(current_time / ul_to_d(runframe), 5) + "ms").c_str());
	ImGui::Text(("Avg. Sim Delta: " + to_str(sim_deltas / ul_to_d(runframe), 5) + "ms").c_str());

	const dvec1 percent = (sim_deltas / current_time) * 100.0;
	ImGui::Text(("~CPU[" + to_str(percent, 2) + "]%%").c_str());
	ImGui::Text(("~GPU[" + to_str(100.0 - percent, 2) + "]%%").c_str());

	ImGui::Text(("Avg. Fps: " + to_str(ul_to_d(runframe) / current_time, 1)).c_str());
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::gameLoop() {
	if (keys[GLFW_MOUSE_BUTTON_RIGHT] or keys[GLFW_MOUSE_BUTTON_LEFT]) {
		const dvec1 xoffset = (last_mouse.x - current_mouse.x) * frame_time * camera_orbit_sensitivity;
		const dvec1 yoffset = (last_mouse.y - current_mouse.y) * frame_time * camera_orbit_sensitivity;
		camera_transform.orbit(dvec3(0), dvec3(yoffset, xoffset, 0.0));
		last_mouse = current_mouse;
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
		frame_time = current_time - last_time;
		last_time = current_time;
		window_time += frame_time;

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

		frame_counter++;
		runframe++;
		if (recompile) {
			glDeleteProgram(buffers["compute"]);
			glDeleteProgram(buffers["display"]);
			buffers["compute"] = computeShaderProgram("Render");
			buffers["display"] = fragmentShaderProgram("Display");
			recompile = false;
		}

		if (window_time > 1.0) {
			frame_count = frame_counter;
			window_time -= 1.0;
			frame_counter = 0;
		}

		guiLoop();
		sim_deltas += sim_delta;

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void Renderer::resize() {
	display_aspect_ratio = u_to_d(display_resolution.x) / u_to_d(display_resolution.y);

	current_mouse = dvec2(display_resolution) / 2.0;
	last_mouse = current_mouse;
}

void Renderer::framebufferSize(GLFWwindow* window, int width, int height) {
	Renderer* instance = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	glViewport(0, 0, width, height);
	instance->display_resolution.x = width;
	instance->display_resolution.y = height;
	instance->resize();
}

void Renderer::cursorPos(GLFWwindow* window, double xpos, double ypos) {
	Renderer* instance = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	instance->current_mouse = dvec2(xpos, ypos);
}

void Renderer::mouseButton(GLFWwindow* window, int button, int action, int mods) {
	Renderer* instance = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	if (action == GLFW_PRESS) {
		instance->keys[button] = true;
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			instance->last_mouse = dvec2(xpos, ypos);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
	}
	else if (action == GLFW_RELEASE) {
		instance->keys[button] = false;
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			instance->last_mouse = dvec2(xpos, ypos);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
}

void Renderer::scroll(GLFWwindow* window, double xoffset, double yoffset) {
	Renderer* instance = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	if (yoffset < 0) {
		instance->camera_transform.moveLocal(dvec3(0.0, 0.0,  25.0) * instance->camera_zoom_sensitivity * instance->frame_time);
	}
	if (yoffset > 0) {
		instance->camera_transform.moveLocal(dvec3(0.0, 0.0, -25.0) * instance->camera_zoom_sensitivity * instance->frame_time);
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
		instance->keys[key] = true;
	}
	else if (action == GLFW_RELEASE) {
		instance->keys[key] = false;
	}
}