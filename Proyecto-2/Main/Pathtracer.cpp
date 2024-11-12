#include "Pathtracer.hpp"

#include "Window.hpp"

PathTracer::PathTracer(Renderer* renderer) :
	renderer(renderer)
{
	texture_size = 0;
	compute_layout = uvec2(0);

	render_planet            = true;
	render_lighting          = true;
	render_atmosphere        = true;

	use_probe_octree         = true;
	use_particle_octree      = true;
	render_probe_lighting    = false;
	render_particle_lighting = false;
	render_probe_octree      = false;
	render_particle_octree   = false;
	{
		render_octree_hue         = false;
		render_octree_debug       = false;
		render_octree_debug_index = 0;
	}
	render_particles = false;
	render_probes    = false;
	{
		render_probe_color_mode = 1;
	}
	render_planet_texture = 0;
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
	gl_data["ssbo 5"] = 0;
	gl_data["ssbo 6"] = 0;

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

	compute_layout.x = d_to_u(ceil(u_to_d(renderer->render_resolution.x) / 16.0));
	compute_layout.y = d_to_u(ceil(u_to_d(renderer->render_resolution.y) / 16.0));

	// Compute Output
	gl_data["raw_render_layer"] = renderLayer(renderer->render_resolution);

	#ifdef NDEBUG
		f_updateTextures(true);
	#else
		f_updateTextures(false);
	#endif // DEBUG
		renderer->f_resize();
}

void PathTracer::f_updateProbes() {
	START_TIMER("Transfer");
	glDeleteBuffers(1, &gl_data["ssbo 1"]);
	glDeleteBuffers(1, &gl_data["ssbo 2"]);
	gl_data["ssbo 1"] = ssboBindingDynamic(ul_to_u(renderer->kernel.gpu_probes.size() * sizeof(GPU_Probe)), renderer->kernel.gpu_probes.data());
	gl_data["ssbo 2"] = ssboBindingDynamic(ul_to_u(renderer->kernel.probe_nodes.size() * sizeof(GPU_Bvh)), renderer->kernel.probe_nodes.data());
	ADD_TIMER("Transfer");
}

// TODO, persistent GPU_DATA to avoid expensive overwrites

void PathTracer::f_updateParticles() {
	START_TIMER("Transfer");
	glDeleteBuffers(1, &gl_data["ssbo 3"]);
	glDeleteBuffers(1, &gl_data["ssbo 4"]);
	gl_data["ssbo 3"] = ssboBindingDynamic(ul_to_u(renderer->kernel.gpu_particles.size() * sizeof(GPU_Particle)), renderer->kernel.gpu_particles.data());
	gl_data["ssbo 4"] = ssboBindingDynamic(ul_to_u(renderer->kernel.particle_nodes.size() * sizeof(GPU_Bvh)), renderer->kernel.particle_nodes.data());
	ADD_TIMER("Transfer");
}

void PathTracer::f_updateTextures(const bool& high_res) {
	const string LR = high_res ? "" : " LR";
	vector<uint> texture_data;
	vector<string> texture_names = {
		"Blue Marble",
		"Black Marble",
		"Topography",
		"Bathymetry",
		"Pressure CAF",
		"Sea Surface Temperature" + LR,
		"Sea Surface Temperature Night" + LR,
		"Land Surface Temperature" + LR,
		"Land Surface Temperature Night" + LR,
		"Humidity",
		"Water Vapor" + LR,
		"Cloud Fraction CAF",
		"Cloud Water Content" + LR,
		"Cloud Particle Radius" + LR,
		"Cloud Optical Thickness" + LR,
		"Ozone" + LR,
		"Albedo" + LR,
		"UV Index",
		"Net Radiation",
		"Solar Insolation",
		"Outgoing Longwave Radiation",
		"Reflected Shortwave Radiation"
	};
	for (const string& tex : texture_names) {
		Texture texture = Texture::fromFile("./Resources/Data/" + tex + ".png", Texture_Format::RGBA_8);
		textures.push_back(GPU_Texture(ul_to_u(texture_data.size()), texture.resolution.x, texture.resolution.y, 0));
		auto data = texture.toRgba8Texture();
		texture_data.insert(texture_data.end(), data.begin(), data.end());
	}
	texture_size = ul_to_u(texture_data.size());
	gl_data["ssbo 5"] = ssboBinding(ul_to_u(textures.size() * sizeof(GPU_Texture)), textures.data());
	gl_data["ssbo 6"] = ssboBinding(ul_to_u(texture_data.size() * sizeof(uint)), texture_data.data());
}

void PathTracer::f_guiUpdate(const vec1& availableWidth, const vec1& spacing, const vec1& itemWidth, const vec1& halfWidth, const vec1& thirdWidth, const vec1& halfPos) {
	ImGui::PushItemWidth(itemWidth);

	if (ImGui::CollapsingHeader("Pathtracing Optimization Settings")) {
		ImGui::Text("Render Scale");
		if (ImGui::SliderFloat("##render_scale", &renderer->render_scale, 0.05f, 1.0f, "%.3f")) {
			renderer->f_resize();
		}
		ImGui::Checkbox("Use Probe Octree", &use_probe_octree);
		ImGui::Checkbox("Use Particle Octree", &use_particle_octree);
		if (!renderer->run_sim and use_probe_octree and render_probe_color_mode < SPH) {
			int MAX_OCTREE_DEPTH = u_to_i(renderer->kernel.PROBE_MAX_OCTREE_DEPTH);
			ImGui::Text("Max Probe Octree Depth");
			if (ImGui::SliderInt("##PROBE_MAX_OCTREE_DEPTH", &MAX_OCTREE_DEPTH, 0, 6)) {
				renderer->kernel.PROBE_MAX_OCTREE_DEPTH = i_to_u(MAX_OCTREE_DEPTH);
				renderer->kernel.updateGPUProbes();
				f_updateProbes();
			}
		}
		if (!renderer->run_sim and use_particle_octree) {
			int MAX_OCTREE_DEPTH = u_to_i(renderer->kernel.PARTICLE_MAX_OCTREE_DEPTH);
			ImGui::Text("Max Particle Octree Depth");
			if (ImGui::SliderInt("##PARTICLE_MAX_OCTREE_DEPTH", &MAX_OCTREE_DEPTH, 0, 6)) {
				renderer->kernel.PARTICLE_MAX_OCTREE_DEPTH = i_to_u(MAX_OCTREE_DEPTH);
				renderer->kernel.updateGPUParticles();
				f_updateParticles();
			}
		}
	}
	if (ImGui::CollapsingHeader("Pathtraced Rendering")) {
		ImGui::Checkbox("Render Planet", &render_planet);
		if (render_planet) {
			const char* items_b[] = { "Earth", "Blue Marble", "Black Marble", "Topography", "Bathymetry", "Surface Pressure", "Sea Temperature Day", "Sea Temperature Night", "Land Temperature Day", "Land Temperature Night", "Humidity", "Water Vapor", "Cloud Coverage", "Cloud Water Content", "Cloud Particle Radius", "Cloud Optical Thickness", "Ozone", "Albedo", "UV Index", "Net Radiation", "Solar Insolation", "Outgoing Longwave Radiation", "Reflected Shortwave Radiation" };
			ImGui::Text("Planet Texture");
			ImGui::Combo("##render_planet_texture", &render_planet_texture, items_b, IM_ARRAYSIZE(items_b));
		}

		ImGui::Checkbox("Render Lighting", &render_lighting);
		if (render_lighting and render_probe_color_mode < SPH) {
			ImGui::Checkbox("Render Atmosphere", &render_atmosphere);
		}

		if (!renderer->run_sim) {
			if (use_probe_octree) {
				if (ImGui::Checkbox("Render Probe Octree", &render_probe_octree)) {
					render_octree_debug_index = 0;
					render_particle_octree = false;
				}
				if (render_probe_octree) {
					ImGui::Checkbox("Hue", &render_octree_hue);
					ImGui::Checkbox("Debug", &render_octree_debug);
					if (render_octree_debug) {
						ImGui::Text("Index");
						ImGui::SliderInt("##render_octree_debug_index", &render_octree_debug_index, 0, ul_to_i(renderer->kernel.probe_nodes.size()));
					}
				}
			}
			if (use_particle_octree) {
				if (ImGui::Checkbox("Render Particle Octree", &render_particle_octree)) {
					render_octree_debug_index = 0;
					render_probe_octree = false;
				}
				if (render_particle_octree) {
					ImGui::Checkbox("Hue", &render_octree_hue);
					ImGui::Checkbox("Debug", &render_octree_debug);
					if (render_octree_debug) {
						ImGui::Text("Index");
						ImGui::SliderInt("##render_octree_debug_index", &render_octree_debug_index, 0, ul_to_i(renderer->kernel.particle_nodes.size()));
					}
				}
			}
		}

		ImGui::Checkbox("Render Probes", &render_probes);
		if (render_probes) {
			ImGui::Checkbox("Render Probe Lighting", &render_probe_lighting);
			if (render_probe_color_mode < SPH) {
				ImGui::Text("Probe Display Radius");
				if (ImGui::SliderFloat("##PROBE_RADIUS", &renderer->kernel.PROBE_RADIUS, 0.005f, 0.25f, "%.4f")) {
					renderer->kernel.updateGPUProbes();
					f_updateProbes();
				}
			}

			const char* items_b[] = { "Sun Intensity", "Wind", "Height", "Pressure", "Current Temperature", "Day Temperature", "Night Temperature", "Humidity", "Water Vapor", "Cloud Coverage", "Cloud Water Content", "Cloud Particle Radius", "Cloud Optical Thickness", "Ozone", "Albedo", "UV Index", "Net Radiation", "Solar Insolation", "Outgoing Longwave Radiation", "Reflected Shortwave Radiation", "SPH.Wind", "SPH.Pressure", "SPH.Temperature" };
			ImGui::Text("Probe Color Mode");
			if (ImGui::Combo("##render_probe_color_mode", &render_probe_color_mode, items_b, IM_ARRAYSIZE(items_b))) {
				if (render_probe_color_mode < SPH) {
					renderer->kernel.BVH_SPH = false;
				}
				else {
					renderer->kernel.BVH_SPH = true;
				}
				renderer->kernel.updateGPUProbes();
				f_updateProbes();
			}
		}
		ImGui::Checkbox("Render Particles", &render_particles);
		if (render_particles) {
			ImGui::Checkbox("Render Particle Lighting", &render_particle_lighting);
			ImGui::Text("Particle Display Radius");
			if (ImGui::SliderFloat("##PARTICLE_RADIUS", &renderer->kernel.PARTICLE_RADIUS, 0.005f, 0.25f, "%.4f")) {
				renderer->kernel.updateGPUParticles();
				f_updateParticles();
			}
		}
	}
	ImGui::PopItemWidth();

	if (ImGui::CollapsingHeader("VRAM Stats")) {
		ImGui::Text(string("Octree VRAM: " + to_string((renderer->kernel.probe_nodes.size() * sizeof(GPU_Bvh)) / 1024) + "kb ").c_str());
		ImGui::Text(string("Texture VRAM: " + to_string((texture_size * sizeof(uint)) / 1024 / 1024) + "mb ").c_str());
		ImGui::Text(string("Particle VRAM: " + to_string((renderer->kernel.PROBE_COUNT * sizeof(GPU_Probe)) / 1024) + "kb ").c_str());
	}
}

void PathTracer::f_recompile() {
	{
		auto confirmation = computeShaderProgram("Rendering/Compute");
		if (confirmation) {
			glDeleteProgram(gl_data["compute_program"]);
			gl_data["compute_program"] =  confirmation.data;
		}
	}
	{
		auto confirmation = fragmentShaderProgram("Rendering/Display", "Rendering/Display");
		if (confirmation) {
			glDeleteProgram(gl_data["display_program"]);
			gl_data["display_program"] = confirmation.data;
		}
	}
	//{
	//	auto confirmation = computeShaderProgram("Compute/Compute");
	//	if (confirmation) {
	//		renderer->kernel.compute_program = confirmation.data;
	//	}
	//}
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
	glDeleteBuffers(1, &gl_data["ssbo 5"]);
	glDeleteBuffers(1, &gl_data["ssbo 6"]);

	glDeleteTextures(1, &gl_data["raw_render_layer"]);

	glBindTextureUnit(0, 0);

	glUseProgram(0);
}

void PathTracer::f_resize() {
	compute_layout.x = d_to_u(ceil(u_to_d(renderer->render_resolution.x) / 8.0));
	compute_layout.y = d_to_u(ceil(u_to_d(renderer->render_resolution.y) / 8.0));

	glDeleteTextures(1, &gl_data["raw_render_layer"]);
	gl_data["raw_render_layer"] = renderLayer(renderer->render_resolution);
}

void PathTracer::f_render() {
	const GLuint compute_program = gl_data["compute_program"];
	const GLuint display_program = gl_data["display_program"];

	const vec1 focal_length = 0.05f;
	const vec1 sensor_size  = 0.036f;

	const vec3 worldUp = vec3(0, 1, 0);
	const vec3 rotatedDirection = d_to_f(renderer->camera) * vec3(0, 0, 1);
	const vec3 camera_pos = rotatedDirection * d_to_f(renderer->zoom);
	const vec3 z_vector = -glm::normalize(camera_pos);
	const vec3 y_vector = d_to_f(renderer->camera) * vec3(0, 1, 0);
	const vec3 projection_center = camera_pos + focal_length * z_vector;
	const vec3 projection_u = normalize(cross(z_vector, y_vector)) * sensor_size;
	const vec3 projection_v = normalize(cross(projection_u, z_vector)) * sensor_size;

	//const mat4 matrix = d_to_f(glm::yawPitchRoll(renderer->camera_transform.euler_rotation.y * DEG_RAD, renderer->camera_transform.euler_rotation.x * DEG_RAD, renderer->camera_transform.euler_rotation.z * DEG_RAD));
	//const vec3 y_vector = matrix[1];
	//const vec3 z_vector = -matrix[2];
	//const vec3 camera_pos = d_to_f(renderer->camera_transform.position);
	//const vec3 projection_center = camera_pos + focal_length * z_vector;
	//const vec3 projection_u = normalize(cross(z_vector, y_vector)) * sensor_size;
	//const vec3 projection_v = normalize(cross(projection_u, z_vector)) * sensor_size;

	glUseProgram(compute_program);

	glUniform1ui (glGetUniformLocation(compute_program, "frame_count"), ul_to_u(renderer->runframe));
	glUniform1f  (glGetUniformLocation(compute_program, "aspect_ratio"), d_to_f(renderer->render_aspect_ratio));
	glUniform1f  (glGetUniformLocation(compute_program, "current_time"), d_to_f(chrono::duration<double>(renderer->current_time - renderer->start_time).count()));
	glUniform2ui (glGetUniformLocation(compute_program, "resolution"), renderer->render_resolution.x, renderer->render_resolution.y);
	glUniform3fv (glGetUniformLocation(compute_program, "camera_pos"), 1, value_ptr(camera_pos));
	glUniform3fv (glGetUniformLocation(compute_program, "camera_p_uv"),1, value_ptr(projection_center));
	glUniform3fv (glGetUniformLocation(compute_program, "camera_p_u"), 1, value_ptr(projection_u));
	glUniform3fv (glGetUniformLocation(compute_program, "camera_p_v"), 1, value_ptr(projection_v));
	glUniform1f  (glGetUniformLocation(compute_program, "earth_tilt"),d_to_f( renderer->kernel.EARTH_TILT));
	glUniform1f  (glGetUniformLocation(compute_program, "year_time"), d_to_f(renderer->kernel.YEAR_TIME));
	glUniform1f  (glGetUniformLocation(compute_program, "day_time"),  d_to_f(renderer->kernel.DAY_TIME));
	glUniform1ui (glGetUniformLocation(compute_program, "use_probe_octree"), use_probe_octree);
	glUniform1ui (glGetUniformLocation(compute_program, "use_particle_octree"), use_particle_octree);
	glUniform1ui (glGetUniformLocation(compute_program, "render_planet"), render_planet);
	glUniform1i  (glGetUniformLocation(compute_program, "render_planet_texture"), render_planet_texture);
	glUniform1ui (glGetUniformLocation(compute_program, "render_lighting"), render_lighting);
	glUniform1ui (glGetUniformLocation(compute_program, "render_probe_lighting"), render_probe_lighting);
	glUniform1ui (glGetUniformLocation(compute_program, "render_particle_lighting"), render_particle_lighting);
	glUniform1ui (glGetUniformLocation(compute_program, "render_atmosphere"), render_atmosphere);
	glUniform1ui (glGetUniformLocation(compute_program, "render_probe_octree"), render_probe_octree);
	glUniform1ui (glGetUniformLocation(compute_program, "render_particle_octree"), render_particle_octree);
	glUniform1ui (glGetUniformLocation(compute_program, "render_octree_hue"), render_octree_hue);
	glUniform1ui (glGetUniformLocation(compute_program, "render_octree_debug"), render_octree_debug);
	glUniform1i  (glGetUniformLocation(compute_program, "render_octree_debug_index"), render_octree_debug_index);
	glUniform1ui (glGetUniformLocation(compute_program, "render_probes"), render_probes);
	glUniform1f  (glGetUniformLocation(compute_program, "render_probe_radius"), renderer->kernel.PROBE_RADIUS);
	glUniform1i  (glGetUniformLocation(compute_program, "render_probe_color_mode"), render_probe_color_mode);
	glUniform1ui (glGetUniformLocation(compute_program, "render_particles"), render_particles);
	glUniform1f  (glGetUniformLocation(compute_program, "render_particle_radius"), renderer->kernel.PARTICLE_RADIUS);

	glBindImageTexture(0, gl_data["raw_render_layer"], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gl_data["ssbo 1"]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, gl_data["ssbo 2"]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, gl_data["ssbo 3"]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, gl_data["ssbo 4"]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, gl_data["ssbo 5"]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, gl_data["ssbo 6"]);

	glDispatchCompute(compute_layout.x, compute_layout.y, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	glUseProgram(display_program);
	glClear(GL_COLOR_BUFFER_BIT);
	glUniform1f  (glGetUniformLocation(display_program, "display_aspect_ratio"), d_to_f(renderer->display_aspect_ratio));
	glUniform1f  (glGetUniformLocation(display_program, "render_aspect_ratio") , d_to_f(renderer->render_aspect_ratio));
	glUniform2uiv(glGetUniformLocation(display_program, "display_resolution"), 1, value_ptr(renderer->display_resolution));
	glUniform2uiv(glGetUniformLocation(display_program, "render_resolution") , 1, value_ptr(renderer->render_resolution));
	bindRenderLayer(display_program, 0, gl_data["raw_render_layer"], "raw_render_layer");
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glUseProgram(0);
}