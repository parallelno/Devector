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


auto dev::WinGlUtils::InitShader(const HWND _hWnd,
	const char* _vtxShaderS, const char* _fragShaderS)
-> Id
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return {};
	auto& gfxContext = it->second;
	CurrentGfxContext currentGfxContext{ gfxContext };

	return m_gLUtilsP->InitShader(_vtxShaderS, _fragShaderS);
}


auto dev::WinGlUtils::InitMaterial(const HWND _hWnd,
		const Id _shaderId, const GLUtils::TextureIds& _textureIds, 
		const GLUtils::ShaderParams& _shaderParams,
		const int _framebufferW, const int _framebufferH, 
		const bool _renderToTexture,
		const int _framebufferTextureFilter)
-> Id
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return {};
	auto& gfxContext = it->second;
	CurrentGfxContext currentGfxContext{ gfxContext };

	return m_gLUtilsP->InitMaterial( _shaderId,
		_textureIds, _shaderParams,
		_framebufferW, _framebufferH, _renderToTexture,
		_framebufferTextureFilter);
}


auto dev::WinGlUtils::InitTexture(const HWND _hWnd,
		const GLsizei _w, const GLsizei _h, 
		const GLUtils::Texture::Format _format,
		const GLint _textureFilter) 
-> Id
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return {};
	auto& gfxContext = it->second;
	CurrentGfxContext currentGfxContext{ gfxContext };

	return m_gLUtilsP->InitTexture(_w, _h, _format, _textureFilter);
}


void dev::WinGlUtils::UpdateTexture(const HWND _hWnd, const Id _texureId, const uint8_t* _memP)
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return;
	auto& gfxContext = it->second;
	CurrentGfxContext currentGfxContext{ gfxContext };

	m_gLUtilsP->UpdateTexture(_texureId, _memP);
}

auto dev::WinGlUtils::Draw(const HWND _hWnd, const Id _materialId,
	const GLsizei _viewportW, const GLsizei _viewportH) const
-> dev::ErrCode
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return ErrCode::UNSPECIFIED;
	auto& gfxContext = it->second;
	CurrentGfxContext currentGfxContext{ gfxContext };

	auto matP = m_gLUtilsP->GetMaterial(_materialId);
	if (matP) {
		matP->viewportW = _viewportW;
		matP->viewportH = _viewportH;
	}

	m_gLUtilsP->Draw(_materialId);

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

	return Status::INITED;
}