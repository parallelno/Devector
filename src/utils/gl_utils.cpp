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

dev::GLUtils::GLUtils()
{
#ifndef WPF
	// Initialize GLAD
	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) 
	{
		dev::Log("Failed to initialize GLAD");
		return;  // Exit if GLAD failed to initialize
	}
	m_status = Status::INITED;

	InitGeometry();
#endif
}

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



// init OpenGL context
#ifdef WPF
auto dev::GLUtils::CreateGfxContext(HWND _hWnd, GLsizei _viewportW, GLsizei _viewportH)
-> std::tuple<Status, GfxContext>
{
	GfxContext gfxContext;

	gfxContext.hWnd = _hWnd;
	gfxContext.viewportW = _viewportW;
	gfxContext.viewportH = _viewportH;
	gfxContext.hdc = GetDC(_hWnd);

	if (gfxContext.hdc == nullptr) return { Status::FAILED_DC, gfxContext }; // Failed to get device context

	// Set the pixel format to a format compatible with OpenGL
	int pixelFormat;
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0
	};
	pixelFormat = ChoosePixelFormat(gfxContext.hdc, &pfd);
	if (pixelFormat == 0)
	{
		return { Status::FAILED_PIXEL_FORMAT, gfxContext }; // Failed to choose pixel format
	}
	if (!SetPixelFormat(gfxContext.hdc, pixelFormat, &pfd)) {
		return { Status::FAILED_SET_PIXEL_FORMAT, gfxContext }; //Failed to set pixel format
	}

	// Create an OpenGL render context
	gfxContext.hglrc = wglCreateContext(gfxContext.hdc);
	if (gfxContext.hglrc == nullptr) {
		return { Status::FAILED_GL_CONTEXT, gfxContext }; //Failed to create OpenGL context
	}

	return { Status::INITED, gfxContext };
}


auto dev::GLUtils::Init(HWND _hWnd, GLsizei _viewportW, GLsizei _viewportH)
-> Status
{
	if (m_gfxContext.hWnd == nullptr)
	{
		auto [status, gfxContextNew] = CreateGfxContext(_hWnd, _viewportW, _viewportH);

		if (status != Status::INITED)
		{
			gfxContextNew.Release();
			return status;
		}

		m_gfxContext = gfxContextNew;
	}

	// Make the OpenGl render context current
	CurrentGfxContext currentGfxContext{ m_gfxContext };

	// Initialize GLAD
	if (!gladLoadGL())
	{
		m_gfxContext.Release();
		return Status::FAILED_GLAD;
	}

	InitGeometry();

	return Status::INITED;
}

void dev::GLUtils::GfxContext::Release()
{
	wglMakeCurrent(nullptr, nullptr);
	if (hglrc) wglDeleteContext(hglrc);
	if (hdc) ReleaseDC(hWnd, hdc);

	hWnd = nullptr;
	hdc = nullptr;
	hglrc = nullptr;
}
#endif


dev::GLUtils::~GLUtils()
{
	if (m_status != Status::INITED) return;

	for (const auto& [id, material] : m_materials)
	{
		glDeleteFramebuffers(1, &material.framebuffer);
		glDeleteTextures(1, &material.framebufferTexture);
	}

	for (const auto& [id, texture] : m_textures) {
		glDeleteTextures(1, &id);
	}

	for (const auto id : m_shaders) {
		glDeleteProgram(id);
	}

	glDeleteVertexArrays(1, &vtxArrayObj);
	glDeleteBuffers(1, &vtxBufferObj);

#ifdef WPF
	m_gfxContext.Release();
#endif
}

dev::GLUtils::Material::Material(GLuint _shaderId,
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

auto dev::GLUtils::InitMaterial(
		GLuint _shaderId, const int _framebufferW, const int _framebufferH, 
		const TextureIds& _textureIds, const ShaderParams& _paramParams, 
		const int _framebufferTextureFilter)
-> dev::Result<MaterialId>
{

	if (m_status != Status::INITED) return {};

#ifdef WPF
	CurrentGfxContext currentGfxContext{ m_gfxContext };
#endif
	
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
		
#ifdef WPF 
		MessageBox(m_gfxContext.hWnd, L"Framebuffer is not complete!", L"Error", MB_OK);
#else
		dev::Log("Framebuffer is not complete!");
#endif
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

	if (!IsMaterialReady(materialId)) { return {}; }

	return materialId;
}


bool dev::GLUtils::IsMaterialReady(const int _materialId) const
{
	auto& material = m_materials.at(_materialId);

	bool ready = vtxArrayObj && vtxBufferObj && material.shaderId >= 0 &&
		material.framebuffer >= 0 && material.framebufferTexture >= 0;

	return ready;
}

void dev::GLUtils::UpdateTexture(const int _texureId, const uint8_t* _memP)
{
	//if (_materialId >= m_renderDatas.size()) return;
	auto& texture = m_textures.at(_texureId);

#ifdef WPF
	CurrentGfxContext currentGfxContext{ m_gfxContext };
#endif

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
		const GLint _textureFilter) 
-> Result<GLuint>
{
#ifdef WPF
	CurrentGfxContext currentGfxContext{ m_gfxContext };
#endif

	int id = -1;

	if (_w > 0 && _h > 0)
	{

		Texture texture{ _w, _h, _format, _textureFilter };
		id = texture.id;
		auto p = std::pair{ id , std::move(texture) };

		m_textures.emplace(std::move(p));
	}

	return id < 0 ? Result<GLuint>{} : Result<GLuint>(id);
}

auto dev::GLUtils::GetFramebufferTexture(const int _materialId) const
-> GLuint
{
	return m_materials.at(_materialId).framebufferTexture;
}

auto dev::GLUtils::GLCheckError(GLuint _obj, const std::string& _txt)
-> Result<GLuint>
{
#ifdef WPF
	CurrentGfxContext currentGfxContext{ m_gfxContext };
#endif

	// Check for compilation errors
	GLint success;
	glGetShaderiv(_obj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetShaderInfoLog(_obj, 512, nullptr, infoLog);
#ifdef WPF 
		auto err = std::format("{}:\n{}", _txt, infoLog);
		MessageBox(m_gfxContext.hWnd, dev::StrToStrW(err).c_str(), L"Error", MB_OK);
#else
		dev::Log("{}:\n {}", _txt, std::string(infoLog));
#endif
		return {};
	}

	return _obj;
}

auto dev::GLUtils::CompileShader(GLenum _shaderType, const char* _source)
-> Result<GLuint>
{
	GLuint shader = glCreateShader(_shaderType);
	glShaderSource(shader, 1, &_source, nullptr);
	glCompileShader(shader);

	return GLCheckError(shader, std::format("Shader compilation failed:\n {}", _source));
}

auto dev::GLUtils::InitShader(const char* _vertexShaderSource, const char* _fragmentShaderSource)
-> Result<GLuint>
{
#ifdef WPF
	CurrentGfxContext currentGfxContext{ m_gfxContext };
#endif

	auto programs = m_shaders.size();

	// Compile vertex and fragment shaders
	auto vertexShaderRes = CompileShader(GL_VERTEX_SHADER, _vertexShaderSource);
	if (vertexShaderRes)
	{
		GLuint vertexShader = *vertexShaderRes;

		auto fragmentShaderRes = CompileShader(GL_FRAGMENT_SHADER, _fragmentShaderSource);
		if (fragmentShaderRes)
		{
			GLuint fragmentShader = *fragmentShaderRes;

			// Create shader program
			GLuint shaderProgram = glCreateProgram();
			glAttachShader(shaderProgram, vertexShader);
			glAttachShader(shaderProgram, fragmentShader);
			glLinkProgram(shaderProgram);

			// TODO: figure out if this check is required. if so, fix it
			//auto shaderProgramRes = GLCheckError(shaderProgram, "Shader program linking failed:\n");
			//if (!shaderProgramRes) goto ReleaseOGLC;

			// Delete shaders
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			m_shaders.push_back(shaderProgram);
		}
	}

	return programs == m_shaders.size() ? Result<GLuint>{} : m_shaders.back();
}

auto dev::GLUtils::Draw(const MaterialId _materialId) const
-> dev::ErrCode
{
	if (m_status != Status::INITED || !IsMaterialReady(_materialId)) return dev::ErrCode::UNSPECIFIED;

	auto& material = m_materials.at(_materialId);

#ifdef WPF
	CurrentGfxContext currentGfxContext{ m_gfxContext };
#endif

	glBindFramebuffer(GL_FRAMEBUFFER, material.framebuffer);
	glViewport(0, 0, material.framebufferW, material.framebufferH);
	glClearColor(material.backColor.x, material.backColor.y, material.backColor.z, material.backColor.w);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(material.shaderId);

	// Pass uniform parameters to the shader
	for (const auto& [paramId, paramValue] : material.params)
	{
		glUniform4f(paramId, paramValue->x, paramValue->y, paramValue->z, paramValue->w);
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
	glBindVertexArray(0);
	// Unbind the framebuffer to return to default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return dev::ErrCode::NO_ERRORS;
}