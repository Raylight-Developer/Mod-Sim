#include "OpenGl.hpp"

#include "Session.hpp"

Texture::Texture() {
	resolution = uvec2(0, 0);
	format = Texture_Format::RGBA_8;
}

#include "External/stb_image.h"

bool Texture::loadFromFile(const string& file_path, const Texture_Format& format) { // TODO handle different formats
	this->format = format;
	switch (format) {
		case Texture_Format::RGBA_8: {
			int width, height, channels;
			unsigned char* tex_data = stbi_load(file_path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
			if (tex_data == nullptr) {
				cerr << " Failed to load image: " << file_path << " " << stbi_failure_reason() << endl;
				return false;
			}
			uint64 totalPixels = width * height * 4;
			uint_data.reserve(totalPixels);
			for (uint64 i = 0; i < totalPixels; ++i) {
				uint_data.push_back(static_cast<uint>(tex_data[i]));
			}

			resolution = uvec2(i_to_u(width), i_to_u(height));
			stbi_image_free(tex_data);
			return true;
		}
		case Texture_Format::MONO_FLOAT: {
			int width, height, channels;
			unsigned char* tex_data = stbi_load(file_path.c_str(), &width, &height, &channels, STBI_grey);
			if (tex_data == nullptr) {
				cerr << " Failed to load image: " << file_path << " " << stbi_failure_reason() << endl;
				return false;
			}
			uint64 totalPixels = width * height;
			float_data.reserve(totalPixels);
			for (uint64 i = 0; i < totalPixels; ++i) {
				float_data.push_back(u_to_f(static_cast<uint>(tex_data[i])) / 255.0f);
			}

			resolution = uvec2(i_to_u(width), i_to_u(height));
			stbi_image_free(tex_data);
			return true;
		}
	}
	return false;
}

Texture Texture::fromFile(const string& file_path, const Texture_Format& format) {
	Texture tex;
	tex.loadFromFile(file_path, format);
	return tex;
}

vec1 Texture::sampleTextureMono(const vec2& uv, const Texture_Format& format) const {
	switch (format) {
		case Texture_Format::MONO_FLOAT: {
			const uint x = clamp(uint(uv.x * vec1(resolution.x)), 0u, resolution.x - 1);
			const uint y = clamp(uint(uv.y * vec1(resolution.y)), 0u, resolution.y - 1);
			const uint index = y * resolution.x + x;
			return clamp(float_data[index], 0.0f, 1.0f);
		}
	}
	return 0;
}

vec4 Texture::sampleTexture(const vec2& uv, const Texture_Format& format) const {
	switch (format) {
		case Texture_Format::RGBA_8: {
			const uint x = clamp(uint(uv.x * vec1(resolution.x)), 0u, resolution.x - 1);
			const uint y = clamp(uint(uv.y * vec1(resolution.y)), 0u, resolution.y - 1);
			const uint index = y * resolution.x + x;
			const vec4 rgba = vec4(
				vec1(uint_data[index * 4]) / 255.0f,
				vec1(uint_data[index * 4 + 1]) / 255.0f,
				vec1(uint_data[index * 4 + 2]) / 255.0f,
				vec1(uint_data[index * 4 + 3]) / 255.0f
			);
			return clamp(rgba, 0.0f, 1.0f);
		}
	}
	return vec4(1, 0, 1, 1);
}

vector<uint> Texture::toRgba8Texture() const {
	switch (format) {
		case Texture_Format::RGBA_8: {
			vector<uint> packedData;
			for (uint i = 0; i < resolution.x * resolution.y; i++) {
				uint r = uint_data[i * 4 + 0];
				uint g = uint_data[i * 4 + 1];
				uint b = uint_data[i * 4 + 2];
				uint a = uint_data[i * 4 + 3];
				uint rgba = (r << 24) | (g << 16) | (b << 8) | a;
				packedData.push_back(rgba);
			}
			return packedData;
		}
		case Texture_Format::MONO_FLOAT: {
			vector<uint> packedData;
			for (uint i = 0; i < resolution.x * resolution.y; i++) {
				packedData.push_back(*reinterpret_cast<const uint*>(&float_data[i]));
			}
			return packedData;
		}
	}
	return vector<uint>();
}

GPU_Texture::GPU_Texture(const uint& start, const uint& width, const uint& height, const uint& format) :
	start(start),
	width(width),
	height(height),
	format(format)
{}

Confirm<GLuint> fragmentShaderProgram(const string& vert_file_path, const string& frag_file_path) {
	GLuint shader_program = glCreateShader(GL_VERTEX_SHADER);

	GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
	const string vertex_code = loadFromFile("./Resources/Shaders/" + vert_file_path + ".vert");
	const char* vertex_code_cstr = vertex_code.c_str();
	glShaderSource(vert_shader, 1, &vertex_code_cstr, NULL);
	glCompileShader(vert_shader);

	if (!checkShaderCompilation(vert_shader, vertex_code)) {
		return Confirm<GLuint>();
	}

	GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	const string fragment_code = loadFromFile("./Resources/Shaders/" + frag_file_path + ".frag");
	const char* fragment_code_cstr = fragment_code.c_str();
	glShaderSource(frag_shader, 1, &fragment_code_cstr, NULL);
	glCompileShader(frag_shader);

	if (!checkShaderCompilation(frag_shader, fragment_code)) {
		return  Confirm<GLuint>();
	}

	shader_program = glCreateProgram();
	glAttachShader(shader_program, vert_shader);
	glAttachShader(shader_program, frag_shader);
	glLinkProgram(shader_program);

	checkProgramLinking(shader_program);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	return Confirm(shader_program);
}

Confirm<GLuint> computeShaderProgram(const string& file_path) {
	GLuint shader_program;
	string compute_code = preprocessShader("./Resources/Shaders/" + file_path + ".comp");
#ifdef _DEBUG
	writeToFile("./Resources/Shaders/" + file_path + "_Compiled.comp", compute_code);
#endif
	const char* compute_code_cstr = compute_code.c_str();
	GLuint comp_shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(comp_shader, 1, &compute_code_cstr, NULL);
	glCompileShader(comp_shader);

	if (!checkShaderCompilation(comp_shader, compute_code)) {
		return Confirm<GLuint>();
	}

	shader_program = glCreateProgram();
	glAttachShader(shader_program, comp_shader);
	glLinkProgram(shader_program);

	checkProgramLinking(shader_program);

	glDeleteShader(comp_shader);

	return Confirm(shader_program);
}

GLuint renderLayer(const uvec2& resolution) {
	GLuint ID;
	glCreateTextures(GL_TEXTURE_2D, 1, &ID);
	glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // Bilinear
	glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // Bilinear
	glTextureParameteri(ID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(ID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D (ID, 1, GL_RGBA8, resolution.x,resolution.y);
	return ID;
}

void bindRenderLayer(const GLuint& program_id, const GLuint& unit, const GLuint& id, const string& name) {
	glUniform1i(glGetUniformLocation(program_id, name.c_str()), unit);
	glBindTextureUnit(unit, id);
}

bool checkShaderCompilation(const GLuint& shader, const string& shader_code) {
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
		LOG ENDL ENDL ANSI_R << "[OpenGL]" ANSI_RESET << " Shader Compilation Failed: "; FLUSH;
		printShaderErrorWithContext(shader_code, infoLog);
		return false;
	}
	return true;
}

bool checkProgramLinking(const GLuint& program) {
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
		LOG ENDL ENDL ANSI_R << "[OpenGL]" ANSI_RESET << " Program Linking Failed: " << infoLog; FLUSH;
		return false;
	}
	return true;
}


void printShaderErrorWithContext(const string& shaderSource, const string& errorLog) {
	LOG += 1;
	size_t position = errorLog.find('(');
	if (position == string::npos) {
		LOG ENDL << "Error: Unable to parse error log."; FLUSH;
		LOG -= 1;
		return;
	}

	size_t endPos = errorLog.find(')', position);
	if (endPos == string::npos) {
		LOG ENDL << "Error: Unable to parse error log."; FLUSH;
		LOG -= 1;
		return;
	}

	uint64 lineNumber = str_to_ul(errorLog.substr(position + 1, endPos - position - 1));

	Tokens lines = f_split(shaderSource, "\n");

	uint64 startLine = max(0ULL, lineNumber - 4);
	uint64 endLine = min(lines.size(), lineNumber + 3);

	for (uint64 i = startLine; i < endLine; ++i) {
		LOG ENDL << (i + 1) << ": " << lines[i];
		if (i == lineNumber - 1) {
			LOG ENDL ANSI_R << "^-- Error here: " ANSI_RESET << errorLog;
		}
	}
	LOG -= 1;
	LOG ENDL ENDL;
	FLUSH;
}