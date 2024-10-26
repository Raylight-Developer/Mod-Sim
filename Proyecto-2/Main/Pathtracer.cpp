#include "Pathtracer.hpp"

#include "Window.hpp"

PathTracer::PathTracer(Renderer* renderer) :
	renderer(renderer)
{
	texture_size = 0;

	params_bool["render_planet"]     = true;
	params_bool["render_lighting"]   = false;
	params_bool["render_atmosphere"] = true;
	params_bool["render_octree"]     = false;
	{
		params_bool["render_octree_hue"]        = false;
		params_bool["render_octree_debug"]      = false;
		params_int["render_octree_debug_index"] = 0;
	}
	params_bool["render_particles"] = false;
	{
		params_float["render_particle_radius"] = 0.015f;
		params_int["render_particle_color_mode"] = 0;
	}
	params_int["render_planet_texture"] = 0;
}

void PathTracer::f_initialize() {
	gl_data["VAO"] = 0;
	gl_data["VBO"] = 0;
	gl_data["EBO"] = 0;

	gl_data["compute_program"] = 0;
	gl_data["display_program"] = 0;

	gl_data["compute_layout.x"] = 0;
	gl_data["compute_layout.y"] = 0;

	gl_data["raw_render_layer"] = 0;

	gl_data["ssbo 1"] = 0;
	gl_data["ssbo 2"] = 0;
	gl_data["ssbo 3"] = 0;
	gl_data["ssbo 4"] = 0;

	glClearColor(0, 0, 0, 0);

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
	glCreateVertexArrays(1, &gl_data["VAO"]);
	glCreateBuffers(1, &gl_data["VBO"]);
	glCreateBuffers(1, &gl_data["EBO"]);
	GLuint VAO = gl_data["VAO"];
	GLuint VBO = gl_data["VBO"];
	GLuint EBO = gl_data["EBO"];

	glNamedBufferData(VBO, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glNamedBufferData(EBO, sizeof(indices), indices, GL_STATIC_DRAW);

	glEnableVertexArrayAttrib(VAO, 0);
	glVertexArrayAttribBinding(VAO, 0, 0);
	glVertexArrayAttribFormat(VAO, 0, 2, GL_FLOAT, GL_FALSE, 0);

	glEnableVertexArrayAttrib(VAO, 1);
	glVertexArrayAttribBinding(VAO, 1, 0);
	glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat));

	glVertexArrayVertexBuffer(VAO, 0, VBO, 0, 4 * sizeof(GLfloat));
	glVertexArrayElementBuffer(VAO, EBO);

	glBindVertexArray(gl_data["VAO"]);

	f_recompile();

	compute_layout.x = d_to_u(ceil(u_to_d(renderer->render_resolution.x) / 32.0));
	compute_layout.y = d_to_u(ceil(u_to_d(renderer->render_resolution.y) / 32.0));

	// Compute Output
	gl_data["raw_render_layer"] = renderLayer(renderer->render_resolution);

	vector<uint> texture_data;
	vector<string> texture_names = { "Albedo", "Sea Surface Temperature CAF", "Land Surface Temperature CAF" };
	for (const string& tex : texture_names) {
		Texture texture = Texture::fromFile("./Resources/Nasa Earth Data/" + tex + ".png", Texture_Format::RGBA_8);
		textures.push_back(GPU_Texture(ul_to_u(texture_data.size()), texture.resolution.x, texture.resolution.y, 0));
		auto data = texture.toRgba8Texture();
		texture_data.insert(texture_data.end(), data.begin(), data.end());
	}
	texture_size = ul_to_u(texture_data.size());
	gl_data["ssbo 3"] = ssboBinding(ul_to_u(textures.size() * sizeof(GPU_Texture)), textures.data());
	gl_data["ssbo 4"] = ssboBinding(ul_to_u(texture_data.size() * sizeof(uint)), texture_data.data());
}

void PathTracer::f_tickUpdate() {
	//glDeleteBuffers(1, &data["ssbo 1"]);
	//data["ssbo 1"] = ssboBinding(1, ul_to_u(renderer->kernel.gpu_particles.size() * sizeof(GPU_Particle)), renderer->kernel.gpu_particles.data());
}

void PathTracer::f_changeSettings() {
	glDeleteBuffers(1, &gl_data["ssbo 1"]);
	glDeleteBuffers(1, &gl_data["ssbo 2"]);

	gl_data["ssbo 1"] = ssboBinding(ul_to_u(renderer->kernel.gpu_particles.size() * sizeof(GPU_Particle)), renderer->kernel.gpu_particles.data());
	gl_data["ssbo 2"] = ssboBinding(ul_to_u(renderer->kernel.bvh_nodes.size() * sizeof(GPU_Bvh)), renderer->kernel.bvh_nodes.data());
}

void PathTracer::f_guiUpdate() {
	ImGui::SeparatorText("Pathtraced Rendering");
	ImGui::Checkbox("Render Planet", &params_bool["render_planet"]);
	if (params_bool["render_planet"]) {
		const char* items_b[] = { "Albedo", "Sea Surface Temperature", "Land Surface Temperature", "Pressure Map 1", "Pressure Map 2", "Pressure Map 3", "Pressure Map 4", "Wind Map" };
		ImGui::Combo("Planet Texture", &params_int["render_planet_texture"], items_b, IM_ARRAYSIZE(items_b));
	}

	ImGui::Checkbox("Render Lighting", &params_bool["render_lighting"]);
	if (params_bool["render_lighting"]) {
		ImGui::Checkbox("Render Atmosphere", &params_bool["render_atmosphere"]);
	}

	ImGui::Checkbox("Render Octree", &params_bool["render_octree"]);
	if (params_bool["render_octree"]) {
		ImGui::Checkbox("Hue", &params_bool["render_octree_hue"]);
		ImGui::Checkbox("Debug", &params_bool["render_octree_debug"]);
		if (params_bool["render_octree_debug"]) {
			ImGui::SliderInt("Index", &params_int["render_octree_debug_index"], 0, ul_to_i(renderer->kernel.bvh_nodes.size()));
		}
	}

	ImGui::Checkbox("Render Particles", &params_bool["render_particles"]);
	if (params_bool["render_particles"]) {
		ImGui::SliderFloat("Particle Radius", &params_float["render_particle_radius"], 0.005f, 0.025f, "%.4f");

		const char* items_b[] = { "Temperature" };
		ImGui::Combo("Particle Color Mode", &params_int["render_particle_color_mode"], items_b, IM_ARRAYSIZE(items_b));
	}

	ImGui::SeparatorText("Pathtracing Optimization Settings");
	if (ImGui::SliderFloat("Render Scale", &renderer->params_float["RENDER_SCALE"], 0.1f, 1.0f, "%.3f")) {
		f_resize();
	}
	if (!renderer->lock_settings) {
		if (ImGui::SliderInt("Max Octree Depth", &renderer->params_int["MAX_OCTREE_DEPTH"], 0, 5)) {
			renderer->kernel.initBvh();
			f_changeSettings();
		}
	}

	ImGui::SeparatorText("Theoretical Performance Stats");

	ImGui::Text(string("Octree VRAM: " + to_string((renderer->kernel.bvh_nodes.size() * sizeof(GPU_Bvh)) / 1024) + "mb ").c_str());
	ImGui::Text(string("Texture VRAM: " + to_string((texture_size * sizeof(uint)) / 1024) + "mb ").c_str());
	ImGui::Text(string("Particle VRAM: " + to_string((renderer->params_int["PARTICLE_COUNT"] * sizeof(GPU_Particle)) / 1024) + "mb ").c_str());
}

void PathTracer::f_recompile() {
	{
		auto confirmation = computeShaderProgram("Compute/Compute");
		if (confirmation) {
			glDeleteProgram(gl_data["compute_program"]);
			gl_data["compute_program"] =  confirmation.data;
		}
	}
	{
		auto confirmation = fragmentShaderProgram("Compute/Display", "Compute/Display");
		if (confirmation) {
			glDeleteProgram(gl_data["display_program"]);
			gl_data["display_program"] = confirmation.data;
		}
	}
}

void PathTracer::f_cleanup() {
	glDeleteVertexArrays(1, &gl_data["VAO"]);
	glDeleteBuffers(1, &gl_data["VBO"]);
	glDeleteBuffers(1, &gl_data["EBO"]);

	glDeleteProgram(gl_data["compute_program"]);
	glDeleteProgram(gl_data["display_program"]);

	glDeleteBuffers(1, &gl_data["ssbo 1"]);
	glDeleteBuffers(1, &gl_data["ssbo 2"]);
	glDeleteBuffers(1, &gl_data["ssbo 3"]);
	glDeleteBuffers(1, &gl_data["ssbo 4"]);

	glDeleteTextures(1, &gl_data["raw_render_layer"]);

	glBindTextureUnit(0, 0);

	glUseProgram(0);
}

void PathTracer::f_resize() {
	compute_layout.x = d_to_u(ceil(u_to_d(renderer->render_resolution.x) / 32.0));
	compute_layout.y = d_to_u(ceil(u_to_d(renderer->render_resolution.y) / 32.0));

	glDeleteTextures(1, &gl_data["raw_render_layer"]);
	gl_data["raw_render_layer"] = renderLayer(renderer->render_resolution);
}

void PathTracer::f_render() {
	const GLuint compute_program = gl_data["compute_program"];
	const GLuint display_program = gl_data["display_program"];

	const mat4 matrix = d_to_f(glm::yawPitchRoll(renderer->camera_transform.euler_rotation.y * DEG_RAD, renderer->camera_transform.euler_rotation.x * DEG_RAD, renderer->camera_transform.euler_rotation.z * DEG_RAD));
	const vec3 y_vector = matrix[1];
	const vec3 z_vector = -matrix[2];

	const vec1 focal_length = 0.05f;
	const vec1 sensor_size  = 0.036f;

	const vec3 camera_pos = d_to_f(renderer->camera_transform.position);
	const vec3 projection_center = camera_pos + focal_length * z_vector;
	const vec3 projection_u = normalize(cross(z_vector, y_vector)) * sensor_size;
	const vec3 projection_v = normalize(cross(projection_u, z_vector)) * sensor_size;

	glUseProgram(compute_program);

	glUniform1ui (glGetUniformLocation(compute_program, "frame_count"), ul_to_u(renderer->runframe));
	glUniform1f  (glGetUniformLocation(compute_program, "aspect_ratio"), d_to_f(renderer->render_aspect_ratio));
	glUniform1f  (glGetUniformLocation(compute_program, "current_time"), d_to_f(renderer->current_time));
	glUniform2ui (glGetUniformLocation(compute_program, "resolution"), renderer->render_resolution.x, renderer->render_resolution.y);

	glUniform3fv (glGetUniformLocation(compute_program, "camera_pos"), 1, value_ptr(camera_pos));
	glUniform3fv (glGetUniformLocation(compute_program, "camera_p_uv"),1, value_ptr(projection_center));
	glUniform3fv (glGetUniformLocation(compute_program, "camera_p_u"), 1, value_ptr(projection_u));
	glUniform3fv (glGetUniformLocation(compute_program, "camera_p_v"), 1, value_ptr(projection_v));

	glUniform1f  (glGetUniformLocation(compute_program, "earth_tilt"), renderer->params_float["EARTH_TILT"]);
	glUniform1f  (glGetUniformLocation(compute_program, "date_time"), renderer->params_float["DATE_TIME"]);

	glUniform1ui (glGetUniformLocation(compute_program, "render_planet"), params_bool["render_planet"]);
	{
		glUniform1i(glGetUniformLocation(compute_program, "render_planet_texture"), params_int["render_planet_texture"]);
	}
	glUniform1ui (glGetUniformLocation(compute_program, "render_lighting"), params_bool["render_lighting"]);
	glUniform1ui (glGetUniformLocation(compute_program, "render_atmosphere"), params_bool["render_atmosphere"]);
	glUniform1ui (glGetUniformLocation(compute_program, "render_octree"), params_bool["render_octree"]);
	{
		glUniform1ui (glGetUniformLocation(compute_program, "render_octree_hue"), params_bool["render_octree_hue"]);
		glUniform1ui (glGetUniformLocation(compute_program, "render_octree_debug"), params_bool["render_octree_debug"]);
		glUniform1i  (glGetUniformLocation(compute_program, "render_octree_debug_index"), params_bool["render_octree_debug_index"]);
	}
	glUniform1ui (glGetUniformLocation(compute_program, "render_particles"), params_bool["render_particles"]);
	{
		glUniform1f  (glGetUniformLocation(compute_program, "render_particle_radius"), params_float["render_particle_radius"]);
		glUniform1i(glGetUniformLocation(compute_program, "render_particle_color_mode"), params_bool["render_particle_color_mode"]);
	}

	glBindImageTexture(0, gl_data["raw_render_layer"], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gl_data["ssbo 1"]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, gl_data["ssbo 2"]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, gl_data["ssbo 3"]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, gl_data["ssbo 4"]);
	bindRenderLayer(compute_program, 0, renderer->kernel.gl_data["pass1"], "pass_1");
	bindRenderLayer(compute_program, 1, renderer->kernel.gl_data["pass2"], "pass_2");
	bindRenderLayer(compute_program, 2, renderer->kernel.gl_data["pass3"], "pass_3");
	bindRenderLayer(compute_program, 3, renderer->kernel.gl_data["pass4"], "pass_4");
	bindRenderLayer(compute_program, 4, renderer->kernel.gl_data["passW"], "pass_W");

	glDispatchCompute(compute_layout.x, compute_layout.y, 1);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glUseProgram(display_program);
	glClear(GL_COLOR_BUFFER_BIT);
	glUniform1f  (glGetUniformLocation(display_program, "display_aspect_ratio"), d_to_f(renderer->display_aspect_ratio));
	glUniform1f  (glGetUniformLocation(display_program, "render_aspect_ratio") , d_to_f(renderer->render_aspect_ratio));
	glUniform2uiv(glGetUniformLocation(display_program, "display_resolution"), 1, value_ptr(renderer->display_resolution));
	glUniform2uiv(glGetUniformLocation(display_program, "render_resolution") , 1, value_ptr(renderer->render_resolution));
	bindRenderLayer(display_program, 0, gl_data["raw_render_layer"], "raw_render_layer");
	bindRenderLayer(display_program, 1, renderer->kernel.gl_data["pass1"], "pass_1");
	bindRenderLayer(display_program, 2, renderer->kernel.gl_data["pass2"], "pass_2");
	bindRenderLayer(display_program, 3, renderer->kernel.gl_data["pass3"], "pass_3");
	bindRenderLayer(display_program, 4, renderer->kernel.gl_data["pass4"], "pass_4");
	bindRenderLayer(display_program, 5, renderer->kernel.gl_data["passW"], "pass_W");
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glUseProgram(0);
}