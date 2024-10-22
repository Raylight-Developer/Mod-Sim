#include "Rasterizer.hpp"

#include "Window.hpp"

Rasterizer::Rasterizer(Renderer* renderer) :
	renderer(renderer)
{
}

void Rasterizer::f_initialize() {
	data["display_program"] = 0;
	data["mesh_program"] = 0;
	data["curve_program"] = 0;
	data["FBT"] = 0;
	data["FBO"] = 0;
	data["RBO"] = 0;

	data["VAO"] = 0;
	data["VBO"] = 0;
	data["EBO"] = 0;

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	f_recompile();

	// Fullscreen Quad
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

	glCreateVertexArrays(1, &data["VAO"]);
	glCreateBuffers(1, &data["VBO"]);
	glCreateBuffers(1, &data["EBO"]);
	GLuint Array_Object = data["VAO"];
	GLuint Buffer_Object = data["VBO"];
	GLuint EBO = data["EBO"];

	glNamedBufferData(Buffer_Object, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glNamedBufferData(EBO, sizeof(indices), indices, GL_STATIC_DRAW);

	glEnableVertexArrayAttrib(Array_Object, 0);
	glVertexArrayAttribBinding(Array_Object, 0, 0);
	glVertexArrayAttribFormat(Array_Object, 0, 2, GL_FLOAT, GL_FALSE, 0);

	glEnableVertexArrayAttrib(Array_Object, 1);
	glVertexArrayAttribBinding(Array_Object, 1, 0);
	glVertexArrayAttribFormat(Array_Object, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat));

	glVertexArrayVertexBuffer(Array_Object, 0, Buffer_Object, 0, 4 * sizeof(GLfloat));
	glVertexArrayElementBuffer(Array_Object, EBO);

	glBindVertexArray(0);

	// FBO
	glGenFramebuffers(1, &data["FBO"]);
	const GLuint fbo = data["FBO"];
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &data["FBT"]);
	const GLuint fbt = data["FBT"];
	glBindTexture(GL_TEXTURE_2D, fbt);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderer->render_resolution.x, renderer->render_resolution.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbt, 0);

	glGenRenderbuffers(1, &data["RBO"]);
	const GLuint rbo = data["RBO"];
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,  renderer->render_resolution.x,  renderer->render_resolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Rasterizer::f_tickUpdate() {
}

void Rasterizer::f_changeSettings() {
}

void Rasterizer::f_guiUpdate() {
}

void Rasterizer::f_recompile() {
	//{
	//	auto confirmation = fragmentShaderProgram("Raster/Display", "Raster/Display");
	//	if (confirmation) {
	//		data["display_program"] = confirmation.data;
	//	}
	//}
	//{
	//	auto confirmation = fragmentShaderProgram("Raster/Curve", "Raster/Curve");
	//	if (confirmation) {
	//		data["curve_program"] = confirmation.data;
	//	}
	//}
	//{
	//	auto confirmation = fragmentShaderProgram("Raster/Mesh", "Raster/Mesh");
	//	if (confirmation) {
	//		data["mesh_program"] = confirmation.data;
	//	}
	//}
}

void Rasterizer::f_cleanup() {
	glDeleteVertexArrays(1, &data["VAO"]);
	glDeleteBuffers(1, &data["VBO"]);
	glDeleteBuffers(1, &data["EBO"]);

	glDeleteProgram(data["display_program"]);
	glDeleteProgram(data["curve_program"]);
	glDeleteProgram(data["mesh_program"]);

	glDeleteRenderbuffers(1, &data["RBO"]);
	glDeleteFramebuffers(1, &data["FBO"]);
	glDeleteTextures(1, &data["FBT"]);

	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//for (Object* object : FILE->active_scene.pointer->objects) {
	//	auto vao = &gl_data[object]["VAO"];
	//	auto vbo = &gl_data[object]["VBO"];
	//	glDeleteVertexArrays(1, vao);
	//	glDeleteBuffers(1, vbo);
	//}

	gl_triangle_cache.clear();
	gl_data.clear();

	glBindVertexArray(0);
	glUseProgram(0);

	//for (Object* object : FILE->active_scene.pointer->objects) {
	//	object->cpu_update = true;
	//	if (object->data->type == OBJECT::DATA::Type::MESH) {
	//		object->getMesh()->cpu_update = true;
	//	}
	//}
}

void Rasterizer::f_resize() {
	glDeleteRenderbuffers(1, &data["RBO"]);
	glDeleteFramebuffers(1, &data["FBO"]);
	glDeleteTextures(1, &data["FBT"]);

	glGenFramebuffers(1, &data["FBO"]);
	const GLuint fbo = data["FBO"];
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &data["FBT"]);
	const GLuint fbt = data["FBT"];
	glBindTexture(GL_TEXTURE_2D, fbt);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderer->render_resolution.x, renderer->render_resolution.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbt, 0);

	glGenRenderbuffers(1, &data["RBO"]);
	const GLuint rbo = data["RBO"];
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, renderer->render_resolution.x, renderer->render_resolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Rasterizer::f_render() {
	const GLuint display_program = data["display_program"];
	const GLuint curve_program = data["curve_program"];
	const GLuint mesh_program = data["mesh_program"];

	glViewport(0, 0, renderer->render_resolution.x, renderer->render_resolution.y);
	glBindFramebuffer(GL_FRAMEBUFFER, data["FBO"]);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(mesh_program);

	//OBJECT::DATA::Camera* camera = FILE->f_activeCamera()->getCamera();
	glUniform3fv(glGetUniformLocation(mesh_program, "camera_pos" ), 1, value_ptr(d_to_f(renderer->camera_transform.position)));
	//glUniformMatrix4fv(glGetUniformLocation(mesh_program, "view_matrix"), 1, GL_FALSE, value_ptr(d_to_f(camera->glViewMatrix(FILE->f_activeCamera()))));
	//glUniformMatrix4fv(glGetUniformLocation(mesh_program, "projection_matrix"), 1, GL_FALSE, value_ptr(d_to_f(camera->glProjectionMatrix(r_aspect_ratio))));

	//for (Object* object : FILE->active_scene.pointer->objects) {
	//	if (object->data->type == OBJECT::DATA::Type::MESH) {
	//		f_renderMesh(mesh_program, object);
	//	}
	//	else if (object->data->type == OBJECT::DATA::Type::GROUP) {
	//		f_renderGroup(mesh_program, object);
	//	}
	//}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, renderer->display_resolution.x, renderer->display_resolution.y);
	glUseProgram(display_program);

	glUniform1f (glGetUniformLocation(display_program, "display_aspect_ratio"), d_to_f(renderer->display_aspect_ratio));
	glUniform1f (glGetUniformLocation(display_program, "render_aspect_ratio"), d_to_f(renderer->render_aspect_ratio));
	bindRenderLayer(display_program, 0, data["FBT"], "render");

	glBindVertexArray(data["VAO"]);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}