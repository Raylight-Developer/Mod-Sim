#pragma once

#include "Include.hpp"
#include "Ops.hpp"

enum struct Texture_Format {
	RGBA_8,
	MONO_FLOAT
};
struct Texture {
	uvec2 resolution;
	vector<uint> uint_data;
	vector<vec1> float_data;
	Texture_Format format;

	Texture();

	bool loadFromFile(const string& file_path, const Texture_Format& format);
	static Texture fromFile(const string& file_path, const Texture_Format& format);
	vec4 sampleTexture(const vec2& uv, const Texture_Format& format) const;
	vec1 sampleTextureMono(const vec2& uv, const Texture_Format& format) const;

	vector<uint> toRgba8Texture() const;
};

struct alignas(16) GPU_Texture {
	uint start;
	uint width;
	uint height;
	uint format;

	GPU_Texture(
		const uint& start = 0U,
		const uint& width = 0U,
		const uint& height = 0U,
		const uint& format = 0U
	);
};

Confirm<GLuint> fragmentShaderProgram(const string& vert_file_path, const string& frag_file_path);
Confirm<GLuint> computeShaderProgram(const string& file_path);
GLuint renderLayer(const uvec2& resolution);
void   bindRenderLayer(const GLuint& program_id, const GLuint& unit, const GLuint& id, const string& name);
void   copyRenderLayer(const GLuint& source, const GLuint& target, const uvec2& resolution);

bool checkShaderCompilation(const GLuint& shader, const string& shader_code);
bool checkProgramLinking(const GLuint& program);
void printShaderErrorWithContext(const string& shaderSource, const string& errorLog);

template <typename T>
GLuint ssboBinding(const GLuint& size, const T& data) {
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_STATIC_DRAW);
	return buffer;
}
template <typename T>
GLuint ssboBindingDynamic(const GLuint& size, const T& data) {
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_STORAGE_BIT);
	return buffer;
}