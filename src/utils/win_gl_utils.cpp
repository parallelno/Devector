#include <format>

#include "utils/win_gl_utils.h"
#include "utils/result.h"
#include "utils/str_utils.h"


// init OpenGL context
auto dev::WinGlUtils::CreateGfxContext(HWND _hWnd, GLsizei _viewportW, GLsizei _viewportH)
-> Status
{
	auto& gfxContext = m_gfxContexts[_hWnd];
	
	// update a viewport size for a new or existing context
	gfxContext.viewportW = _viewportW;
	gfxContext.viewportH = _viewportH;

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
		gfxContext.hglrcP = &m_hglrc;
		// Make the OpenGl render context current
		CurrentGfxContext currentGfxContext(gfxContext);

		// Initialize GLAD
		if (!gladLoadGL())
		{
			DeleteGfxContext(_hWnd);
			return Status::FAILED_GLAD;
		}

		m_gLUtilsP = std::make_shared<GLUtils>(true);

	}
	
	gfxContext.hglrcP = &m_hglrc;
	gfxContext.hWnd = _hWnd;

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
	auto& gfxContext = it;
	CurrentGfxContext currentGfxContext{ it->second };

	return m_gLUtilsP->InitShader(_vertexShaderSource, _fragmentShaderSource);
}


auto dev::WinGlUtils::InitMaterial(HWND _hWnd,
		GLuint _shaderId, const int _framebufferW, const int _framebufferH, 
		const GLUtils::TextureIds& _textureIds, const GLUtils::ShaderParams& _paramParams,
		const int _framebufferTextureFilter)
-> dev::Result<GLUtils::MaterialId>
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return {};
	auto& gfxContext = it;
	CurrentGfxContext currentGfxContext{ it->second };

	return m_gLUtilsP->InitMaterial( _shaderId,
		_framebufferW, _framebufferH, _textureIds, 
		_paramParams, _framebufferTextureFilter);
}


auto dev::WinGlUtils::InitTexture(HWND _hWnd, 
		GLsizei _w, GLsizei _h, GLUtils::Texture::Format _format,
		const GLint _textureFilter) 
-> Result<GLuint>
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return {};
	auto& gfxContext = it;
	CurrentGfxContext currentGfxContext{ it->second };

	return m_gLUtilsP->InitTexture(_w, _h, _format, _textureFilter);
}


void dev::WinGlUtils::UpdateTexture(HWND _hWnd, const int _texureId, const uint8_t* _memP)
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return;
	auto& gfxContext = it;
	CurrentGfxContext currentGfxContext{ it->second };

	m_gLUtilsP->UpdateTexture(_texureId, _memP);
}



auto dev::WinGlUtils::Draw(HWND _hWnd, const GLUtils::MaterialId _materialId) const
-> dev::ErrCode
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return {};
	auto& gfxContext = it;
	CurrentGfxContext currentGfxContext{ it->second };

	return m_gLUtilsP->Draw(_materialId);
}


auto dev::WinGlUtils::GetFramebufferTexture(HWND _hWnd, const int _materialId) const
-> GLuint
{
	auto it = m_gfxContexts.find(_hWnd);
	if (it == m_gfxContexts.end()) return {};
	auto& gfxContext = it;
	CurrentGfxContext currentGfxContext{ it->second };

	return m_gLUtilsP->GetFramebufferTexture(_materialId);
}