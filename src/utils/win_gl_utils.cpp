#include <format>

#include "utils/win_gl_utils.h"
#include "utils/result.h"
#include "utils/str_utils.h"

// Vertex shader source code
const char* vtxShader1S = R"#(
	#version 330 core
	precision highp float;

	layout (location = 0) in vec3 pos;
	layout (location = 1) in vec2 uv;

	out vec2 uv0;

	uniform vec4 uvScale;

	void main()
	{
		//uv0 = vec2(uv.x, 1.0f - uv.y);
		uv0 = uv;
		uv0 *= uvScale.xy;
		gl_Position = vec4(pos.xyz, 1.0f);
	}
)#";

// Fragment shader source code
const char* fragShader1S = R"#(
	#version 330 core
	precision highp float;
	precision highp int;

	in vec2 uv0;

	uniform sampler2D texture0;

	layout (location = 0) out vec4 out0;

	void main()
	{
		vec2 uv = uv0;
		
		vec3 color = texture(texture0, uv).rgb;

		out0 = vec4(color, 1.0f);
	}
)#";


// init OpenGL context
auto dev::WinGlUtils::CreateGfxContext(HWND _hWnd, const int _framebufferW, const int _framebufferH)
-> Status
{
	auto& gfxContext = m_gfxContexts[_hWnd];

	// if a context exists, return INITED
	if (gfxContext.hWnd) return Status::INITED;

	gfxContext.hdc = GetDC(_hWnd);
	if (gfxContext.hdc == nullptr) 
	{
		DeleteGfxContext(_hWnd);
		return Status::FAILED_DC;
	}

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
		DeleteGfxContext(_hWnd);
		return Status::FAILED_PIXEL_FORMAT;
	}

	if (!SetPixelFormat(gfxContext.hdc, pixelFormat, &pfd)) 
	{
		DeleteGfxContext(_hWnd);
		return Status::FAILED_SET_PIXEL_FORMAT; 
	}

	// Create an OpenGL render context
	if (m_hglrc == nullptr)
	{
		m_hglrc = wglCreateContext(gfxContext.hdc);
		if (m_hglrc == nullptr) 
		{
			DeleteGfxContext(_hWnd);
			return Status::FAILED_GL_CONTEXT;
		}

		// init geometry
		gfxContext.hWnd = _hWnd;
		gfxContext.hglrcP = &m_hglrc;
		
		if (InitRenderObjects(gfxContext, _framebufferW, _framebufferH) != Status::INITED) 
		{
			DeleteGfxContext(_hWnd);
			return Status::FAILED_GL_CONTEXT;
		};
	}
	
	gfxContext.hWnd = _hWnd;
	gfxContext.hglrcP = &m_hglrc;

	return Status::INITED;
}


void dev::WinGlUtils::CleanGfxContext(GfxContext& _gfxContext)
{
	// delete gl objects
	if (m_gfxContexts.size() == 1 && m_hglrc && m_gLUtilsP.get())
	{
		wglMakeCurrent(_gfxContext.hdc, m_hglrc);
		m_gLUtilsP.reset();
	}

	wglMakeCurrent(nullptr, nullptr);

	if (m_gfxContexts.size() == 1 && m_hglrc)
	{
		wglDeleteContext(m_hglrc);
		m_hglrc = nullptr;
	}

	if (_gfxContext.hdc) ReleaseDC(_gfxContext.hWnd, _gfxContext.hdc);

	_gfxContext.hWnd = nullptr;
	_gfxContext.hdc = nullptr;
}


void dev::WinGlUtils::DeleteGfxContext(HWND _hWnd)
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return;

	CleanGfxContext(it->second);
	m_gfxContexts.erase(it);
}


dev::WinGlUtils::~WinGlUtils()
{
	for (auto it = m_gfxContexts.begin(); it != m_gfxContexts.end(); ) 
	{
		CleanGfxContext(it->second);
		it = m_gfxContexts.erase(it);  // Erase and advance iterator
	}
}


auto dev::WinGlUtils::InitShader(HWND _hWnd, 
		const char* _vertexShaderSource, const char* _fragmentShaderSource)
-> Result<GLuint>
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return {};
	auto& gfxContext = it->second;
	CurrentGfxContext currentGfxContext{ gfxContext };

	return m_gLUtilsP->InitShader(_vertexShaderSource, _fragmentShaderSource);
}


auto dev::WinGlUtils::InitMaterial(HWND _hWnd,
		GLuint _shaderId, const GLUtils::TextureIds& _textureIds, const GLUtils::ShaderParams& _paramParams,
		const int _framebufferW, const int _framebufferH, 
		const int _framebufferTextureFilter)
-> dev::Result<GLUtils::MaterialId>
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return {};
	auto& gfxContext = it->second;
	CurrentGfxContext currentGfxContext{ gfxContext };

	return m_gLUtilsP->InitMaterial( _shaderId,
		_textureIds, _paramParams,
		_framebufferW, _framebufferH,
		_framebufferTextureFilter);
}


auto dev::WinGlUtils::InitTexture(HWND _hWnd, 
		GLsizei _w, GLsizei _h, GLUtils::Texture::Format _format,
		const GLint _textureFilter) 
-> Result<GLuint>
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return {};
	auto& gfxContext = it->second;
	CurrentGfxContext currentGfxContext{ gfxContext };

	return m_gLUtilsP->InitTexture(_w, _h, _format, _textureFilter);
}


void dev::WinGlUtils::UpdateTexture(HWND _hWnd, const int _texureId, const uint8_t* _memP)
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return;
	auto& gfxContext = it->second;
	CurrentGfxContext currentGfxContext{ gfxContext };

	m_gLUtilsP->UpdateTexture(_texureId, _memP);
}



auto dev::WinGlUtils::Draw(HWND _hWnd, const GLUtils::MaterialId _materialId) const
-> dev::ErrCode
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return {};
	auto& gfxContext = it->second;
	CurrentGfxContext currentGfxContext{ gfxContext };

	return m_gLUtilsP->Draw(_materialId);
}


auto dev::WinGlUtils::DrawOnWindow(HWND _hWnd, const GLUtils::MaterialId _materialId,
	const GLsizei _viewportW, const GLsizei _viewportH,
	const GLsizei _clippedViewport, const GLsizei _clippedViewportH,
	GLuint _textId) const
-> dev::ErrCode
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return ErrCode::UNSPECIFIED;
	auto& gfxContext = it->second;
	CurrentGfxContext currentGfxContext{ gfxContext };

	auto matP = m_gLUtilsP->GetMaterial(m_matId);
	if (matP) {
		matP->viewportW = _viewportW;
		matP->viewportH = _viewportH;
	}

	// TODO: test
	// _textId
	matP->textureParams.begin()->second = _textId;
	// end test

	m_gLUtilsP->Draw(m_matId);

	if (!SwapBuffers(gfxContext.hdc)) { return ErrCode::UNSPECIFIED; }

	return dev::ErrCode::NO_ERRORS;
}

auto dev::WinGlUtils::InitRenderObjects(const GfxContext& _gfxContext,
	const GLsizei _viewportW, const GLsizei _viewportH)
-> Status
{
	// Make the OpenGl render context current
	CurrentGfxContext currentGfxContext{ _gfxContext };

	// Initialize GLAD
	if (!gladLoadGL()) { return Status::FAILED_GLAD; }

	m_gLUtilsP = std::make_shared<GLUtils>(true);
	if (m_gLUtilsP->IsInited() == 0) {
		return Status::FAILED_INIT_GL_UTILS;
	};	


	auto shaderRes = m_gLUtilsP->InitShader(vtxShader1S, fragShader1S);
	if (!shaderRes) {
		return Status::FAILED_INIT_BLIT_SHADER;
	}
	auto shaderId = *shaderRes;


	auto texRes = m_gLUtilsP->InitTexture(Display::FRAME_W, Display::FRAME_H, GLUtils::Texture::Format::RGBA);
	if (!texRes) return Status::FAILET_INIT_BLIT_TEX;
	auto texId = *texRes;

	GLUtils::ShaderParams shaderParams = {
		{ "uvScale", &m_shaderParamUvScale },
	};

	auto matRes = m_gLUtilsP->InitMaterial(shaderId, 
		{ texId }, shaderParams, _viewportW, _viewportH);

	if (!matRes) return Status::FAILED_INIT_BLIT_MAT;
	m_matId = *matRes;

	return Status::INITED;
}