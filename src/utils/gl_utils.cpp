#include <format>

#include "utils/gl_utils.h"
#include "utils/result.h"
#include "utils/str_utils.h"

GLfloat vertices[] = {
    // First triangle
    -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,  // bottom-left
    1.0f, -1.0f, 0.0f,   1.0f, 1.0f,  // bottom-right
    -1.0f,  1.0f, 0.0f,  0.0f, 0.0f,  // top-left

    // Second triangle
    -1.0f,  1.0f, 0.0f,  0.0f, 0.0f,  // top-left
    1.0f, -1.0f, 0.0f,   1.0f, 1.0f,  // bottom-right
    1.0f,  1.0f, 0.0f,   1.0f, 0.0f   // top-right
};

// it is not initializing the Window and OpenGL 3.3 context
// assumming ImGui and did it already
 dev::GLUtils::GLUtils(bool _init)
 {
	if (!_init) return;
	
#ifndef WPF
	// Initialize GLAD (replaces GLEW initialization)
	m_gladInited = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
	if (!m_gladInited) {
		dev::Log("Failed to initialize GLAD");
		return;  // Exit if GLAD failed to initialize
	}
#else
	m_gladInited = true;
#endif

	InitGeometry();
}

 // init a quad
 void dev::GLUtils::InitGeometry()
 {
	 // Create Vertex Array Object (VAO) and Vertex Buffer Object (VBO)
	 glGenVertexArrays(1, &vtxArrayObj);
	 glGenBuffers(1, &vtxBufferObj);
	 glBindVertexArray(vtxArrayObj);
	 glBindBuffer(GL_ARRAY_BUFFER, vtxBufferObj);
	 // Upload vertex data to the buffer
	 glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	 // Specify layout of vertex data (position)
	 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	 glEnableVertexAttribArray(0);
	 // Specify layout of texture coordinates
	 glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	 glEnableVertexAttribArray(1);
	 // Unbind the buffer and VAO
	 glBindBuffer(GL_ARRAY_BUFFER, 0);
	 glBindVertexArray(0);
 }
// OUTs:
// material ID == 0 : FAIL
 // material ID > 0 : VALID INIT
auto dev::GLUtils::InitMaterial(Id _shaderId, 
		const TextureIds& _textureIds, const ShaderParams& _paramParams,
		const int _framebufferW, const int _framebufferH,
		const bool _renderToTexture,
		const int _framebufferTextureFilter)
-> Id
{
	if (!m_gladInited) return INVALID_ID;
	
	Id materialId = m_materialId++;
	auto& material = m_materials.emplace(materialId, Material{_shaderId, _paramParams, _framebufferW, _framebufferH, _renderToTexture }).first->second;


	if (_renderToTexture)
	{
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
	}

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
	if (!m_gladInited) return;
	
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

auto dev::GLUtils::Draw(const Id _materialId) const
-> ErrCode
{
	if (!m_gladInited || !IsMaterialReady(_materialId)) return ErrCode::UNSPECIFIED;

	auto& material = m_materials.at(_materialId);

	if (material.renderToTexture){
		glBindFramebuffer(GL_FRAMEBUFFER, material.framebuffer);
	}

	glViewport(0, 0, material.viewportW, material.viewportH);
	glClearColor(material.backColor.x, material.backColor.y, material.backColor.z, material.backColor.w);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(material.shaderId);

	// Pass uniform parameters to the shader
	for (const auto& [paramId, paramValue] : material.params)
	{
		glUniform4f(paramId, paramValue.x, paramValue.y, paramValue.z, paramValue.w);
	}

	// Bind textures
	for (const auto& [activateId, id] : material.textureParams)
	{
		glActiveTexture(activateId);
		glBindTexture(GL_TEXTURE_2D, id);
	}
	
	// Bind the VAO and draw the quad
	glBindVertexArray(vtxArrayObj);
	glDrawArrays(GL_TRIANGLES, 0, 6);  // 6 vertices for two triangles

	// Unbind the framebuffer and VAO
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return dev::ErrCode::NO_ERRORS;
}

void dev::GLUtils::UpdateTexture(const Id _texureId, const uint8_t* _memP)
{
	if (_texureId == INVALID_ID) return;

	auto it = m_textures.find(_texureId);
	if (it == m_textures.end()) return;

	auto& texture = it->second;

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

auto dev::GLUtils::GetFramebufferTexture(const Id _materialId) const
-> Id
{
	return m_materials.at(_materialId).framebufferTexture;
}

auto dev::GLUtils::GLCheckError(Id _id, const std::string& _txt)
-> Id
{
	// Check for compilation errors
	GLint success;
	glGetShaderiv(_id, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetShaderInfoLog(_id, 512, NULL, infoLog);
		dev::Log("{}:\n {}", _txt, std::string(infoLog));
		return INVALID_ID;
	}
	return _id;
}

auto dev::GLUtils::CompileShader(GLenum _shaderType, const char* _source)
-> Id
{
	GLuint shader = glCreateShader(_shaderType);
	glShaderSource(shader, 1, &_source, NULL);
	glCompileShader(shader);

	return GLCheckError(shader, std::format("Shader compilation failed:\n {}", _source));
}

auto dev::GLUtils::InitShader(const char* _vtxShaderS, const char* _fragShaderS)
-> Id
{
	// Compile vertex and fragment shaders
	auto vtxShaderId = CompileShader(GL_VERTEX_SHADER, _vtxShaderS);
	if (vtxShaderId == INVALID_ID) return INVALID_ID;

	auto fragShaderId = CompileShader(GL_FRAGMENT_SHADER, _fragShaderS);
	if (fragShaderId ==  INVALID_ID) return INVALID_ID;

	// Create shader program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vtxShaderId);
	glAttachShader(shaderProgram, fragShaderId);
	glLinkProgram(shaderProgram);

	// Delete shaders
	glDeleteShader(vtxShaderId);
	glDeleteShader(fragShaderId);

	m_shaders.push_back(shaderProgram);

	return shaderProgram;
}

bool dev::GLUtils::IsMaterialReady(const Id _materialId) const
{
	auto materialI = m_materials.find(_materialId);
	if (materialI == m_materials.end()) return false;

	bool ready = vtxArrayObj && vtxBufferObj && materialI->second.shaderId >= 0;

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
		const GLint _filter) 
-> Id
{
	if (_w <= 0 || _h <= 0) 
	{
		return INVALID_ID;
	}

	Texture texture{_w, _h, _format, _filter};
	auto id = texture.id;
	auto p = std::pair{ id , std::move(texture) };

	m_textures.emplace(std::move(p));

	return id;
}

dev::GLUtils::Material::Material(Id _shaderId, 
	const ShaderParams& _shaderParams,
	const int _framebufferW, const int _framebufferH,
	const bool _renderToTexture,
	const Vec4& _backColor) 
	:
	shaderId(_shaderId), textureParams(), framebufferTexture(0), framebuffer(0),
	viewportW(_framebufferW), viewportH(_framebufferH), backColor(_backColor),
	renderToTexture(_renderToTexture)
{
	// get uniform vars ids
	for (const auto& [name, val] : _shaderParams)
	{
		auto paramId = glGetUniformLocation(_shaderId, name.c_str());
		if (paramId < 0) continue;

		params[paramId] = val;
		paramIds[name] = paramId;
	}
};

auto dev::GLUtils::Material::GetParamId(const std::string& _paramName)
-> Id
{
	auto paramIt = paramIds.find(_paramName);
	
	return paramIt == paramIds.end() ? INVALID_ID : paramIt->second;
}

auto dev::GLUtils::Material::SetParam(const Id _id, const Vec4& _data)
-> ErrCode
{
	auto paramIt = params.find(_id);

	if (paramIt == params.end()) return ErrCode::INVALID_ID;
	paramIt->second = _data;

	return ErrCode::NO_ERRORS;
}

auto dev::GLUtils::GetMaterial(const Id _matId) 
-> Material*
{
	auto matIdI = m_materials.find(_matId);
	return matIdI == m_materials.end() ? nullptr : &(matIdI->second);
}