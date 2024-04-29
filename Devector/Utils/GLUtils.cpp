#include "GLUtils.h"

#include "Result.h"
#include "Utils/StringUtils.h"
#include <format>
#include "GL/glew.h"

// vertices of a quad with UV coordinates
GLfloat vertices[] = {
	// Positions          // UV Coords
	 -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,
	 -1.0f,  1.0f, 0.0f,  0.0f, 0.0f,
	  1.0f,  1.0f, 0.0f,  1.0f, 0.0f,
	  1.0f, -1.0f, 0.0f,  1.0f, 1.0f,
};

// it is not initializing the Window and OpenGL 3.3 context
// assumming ImGui and did it already

 dev::GLUtils::GLUtils()
 {
	 m_glewInitCode = glewInit();
	 if (m_glewInitCode != GLEW_OK) {
		 dev::Log("Failed to initialize GLEW");
	 }

	// Create Vertex Array Object (VAO) and Vertex Buffer Object (VBO)
	glGenVertexArrays(1, &vtxArrayObj);
	glGenBuffers(1, &vtxBufferObj);
	glBindVertexArray(vtxArrayObj);
	glBindBuffer(GL_ARRAY_BUFFER, vtxBufferObj);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

auto dev::GLUtils::InitMaterial(GLuint _shaderId, const int _framebufferW, const int _framebufferH, 
		const TextureIds& _textureIds, const ShaderParams& _paramParams, 
		const int _framebufferTextureFilter)
-> dev::Result<MaterialId>
{
	if (m_glewInitCode != GLEW_OK) return {};
	
	MaterialId materialId = m_materialId++;
	auto& material = m_materials.emplace(materialId, Material{_shaderId, _framebufferW, _framebufferH, _paramParams}).first->second;

	// Create and bind a framebuffer object (FBO)
	glGenFramebuffers(1, &material.framebuffer);
	
	// Create a framebuffer texture to render to
	glGenTextures(1, &material.framebufferTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, material.framebuffer);
	glBindTexture(GL_TEXTURE_2D, material.framebufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _framebufferW, _framebufferH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _framebufferTextureFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _framebufferTextureFilter);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, material.framebufferTexture, 0);

	// Check framebuffer status
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		dev::Log("Framebuffer is not complete!");
		return {};
	}
	// Unbind framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// activate texture slots
	glUseProgram(_shaderId);
	for (int i = 0; i < _textureIds.size(); i++) 
	{
		auto paramId = glGetUniformLocation(_shaderId, std::format("texture{}", i).c_str());
		if (paramId < 0) continue;
		glUniform1i(paramId, i);
		material.textureParams.emplace(GL_TEXTURE0 + i, _textureIds[i]);
	}

	if (!IsMaterialReady(materialId)) return {};

	return materialId;
}

dev::GLUtils::~GLUtils()
{
	for (const auto& [id, material] : m_materials)
	{
		glDeleteFramebuffers(1, &material.framebuffer);
		glDeleteTextures(1, &material.framebufferTexture);
	}

	for (const auto& [id, texture] : m_textures){
		glDeleteTextures(1, &id);
	}
	
	for (const auto id : m_shaders){
		glDeleteProgram(id);
	}

	glDeleteVertexArrays(1, &vtxArrayObj);
	glDeleteBuffers(1, &vtxBufferObj);
}

int dev::GLUtils::Draw(const MaterialId _materialId) const
{
	if (m_glewInitCode != GLEW_OK ||
		!IsMaterialReady(_materialId)) return -1;

	auto& material = m_materials.at(_materialId);

	glBindFramebuffer(GL_FRAMEBUFFER, material.framebuffer);
	glViewport(0, 0, material.framebufferW, material.framebufferH);
	glClearColor(material.backColor.x, material.backColor.y, material.backColor.z, material.backColor.w);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(material.shaderId);

	// send the params to the shader
	for (const auto& [paramId, paramValue] : material.params)
	{
		glUniform4f(paramId, paramValue->x, paramValue->y, paramValue->z, paramValue->w);
	}

	// bind texture
	for (const auto& [activateId, id] : material.textureParams)
	{
		glActiveTexture(activateId);
		glBindTexture(GL_TEXTURE_2D, id);
	}
	// draw the quad
	glBindVertexArray(vtxArrayObj);
	glDrawArrays(GL_QUADS, 0, 4);
	glBindVertexArray(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return GLEW_OK;
}

void dev::GLUtils::UpdateTexture(const int _texureId, const uint8_t* _memP)
{
	//if (_materialId >= m_renderDatas.size()) return;
	auto& texture = m_textures.at(_texureId);

	glBindTexture(GL_TEXTURE_2D, texture.id);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture.filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture.filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, texture.internalFormat, texture.w, texture.h, 0, texture.internalFormat, texture.type, _memP);
}

auto dev::GLUtils::GetFramebufferTexture(const int _materialId) const
-> GLuint
{
	return m_materials.at(_materialId).framebufferTexture;
}

auto dev::GLUtils::GLCheckError(GLuint1 _obj, const std::string& _txt)
-> Result<GLuint>
{
	// Check for compilation errors
	GLint success;
	glGetShaderiv(_obj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetShaderInfoLog(_obj, 512, NULL, infoLog);
		dev::Log("{}:\n {}", _txt, std::string(infoLog));
		return {};
	}
	return _obj;
}

auto dev::GLUtils::CompileShader(GLenum _shaderType, const char* _source)
-> Result<GLuint>
{
	GLuint shader = glCreateShader(_shaderType);
	glShaderSource(shader, 1, &_source, NULL);
	glCompileShader(shader);

	return GLCheckError(shader, std::format("Shader compilation failed:\n {}", _source));
}

auto dev::GLUtils::InitShader(const char* _vertexShaderSource, const char* _fragmentShaderSource)
-> Result<GLuint>
{
	// Compile vertex and fragment shaders
	auto vertexShaderRes = CompileShader(GL_VERTEX_SHADER, _vertexShaderSource);
	if (!vertexShaderRes) return {};
	GLuint vertexShader = *vertexShaderRes;

	auto fragmentShaderRes = CompileShader(GL_FRAGMENT_SHADER, _fragmentShaderSource);
	if (!fragmentShaderRes) return {};
	GLuint fragmentShader = *fragmentShaderRes;

	// Create shader program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	auto shaderProgramRes = GLCheckError(shaderProgram, "Shader program linking failed:\n");
	if (!shaderProgramRes) return {};

	// Delete shaders
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	m_shaders.push_back(*shaderProgramRes);

	return shaderProgram;
}

bool dev::GLUtils::IsMaterialReady(const int _materialId) const
{
	auto& material = m_materials.at(_materialId);

	bool ready = vtxArrayObj && vtxBufferObj && material.shaderId >= 0 &&
				material.framebuffer >= 0 && material.framebufferTexture >= 0;

	return ready;
}

dev::GLUtils::Texture::Texture(GLsizei _w, GLsizei _h, Format _format, GLint _filter) 
	: format(_format), w(_w), h(_h), filter(_filter), internalFormat(GL_RGB), type(GL_UNSIGNED_BYTE)
{
	switch (_format)
	{
	case Format::RGB:
		internalFormat = GL_RGB;
		type = GL_UNSIGNED_BYTE;
		break;
	case Format::RGBA:
		internalFormat = GL_RGBA;
		type = GL_UNSIGNED_BYTE;
		break;
	case Format::R8:
		internalFormat = GL_RED;
		type = GL_UNSIGNED_BYTE;
		break;
	case Format::R32:
		internalFormat = GL_R32UI;
		type = GL_UNSIGNED_INT;
		break;
	}

	glGenTextures(1, &id);
}

auto dev::GLUtils::InitTexture(GLsizei _w, GLsizei _h, Texture::Format _format, 
		const GLint _filter) -> Result<GLuint>
{
	if (_w <= 0 || _h <= 0) 
	{
		return {};
	}

	Texture texture{_w, _h, _format, _filter};
	auto id = texture.id;
	auto p = std::pair{ id , std::move(texture) };

	m_textures.emplace(std::move(p));

	return id;
}

dev::GLUtils::Material::Material(GLuint1 _shaderId, 
		const int _framebufferW, const int _framebufferH, const ShaderParams& _paramParams,
		const Vec4& _backColor) 
	:
	shaderId(_shaderId), textureParams(), framebufferTexture(0), framebuffer(0),
	framebufferW(_framebufferW), framebufferH(_framebufferH), backColor(_backColor)
{
	// get uniform vars ids
	for (const auto& [name, val] : _paramParams)
	{
		auto paramId = glGetUniformLocation(_shaderId, name.c_str());
		if (paramId < 0) continue;

		params[paramId] = val;
	}
};