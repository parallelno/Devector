#pragma once
#ifdef WPF
#include <glad/glad.h>
#include <SDL3/SDL_opengl.h>

#ifdef APIENTRY
    #undef APIENTRY
#endif

#include <vector>
#include <unordered_map>
#include <tuple>
#include <windows.h>

#include "utils/consts.h"
#include "core/hardware.h"
#include "utils/gl_utils.h"

namespace dev
{
	class WinGlUtils
	{

	public:
		enum class Status { NOT_INITED, INITED, 
			FAILED_DC,  // Failed to get device context
			FAILED_PIXEL_FORMAT, // Failed to choose pixel format
			FAILED_SET_PIXEL_FORMAT, //Failed to set pixel format
			FAILED_GL_CONTEXT, //Failed to create OpenGL context
			FAILED_GLAD, // Initialize to load GLAD lib
		};

	private:

		std::shared_ptr<GLUtils> m_gLUtilsP;

		struct GfxContext
		{
			HWND hWnd = nullptr; // window handle
			HDC hdc = nullptr; // digital context
			HGLRC* hglrcP = nullptr; // shared gl render context

			GLsizei viewportW = 0;
			GLsizei viewportH = 0;
		};

		std::unordered_map<HWND, GfxContext> m_gfxContexts;
		HGLRC m_hglrc = nullptr; // shared opengl render context between WPF controls

		
		void CleanGfxContext(GfxContext& _gfxContext);

		// helper class to auto-release the gfx context when it's out of scope
		struct CurrentGfxContext {
			CurrentGfxContext(const GfxContext& _gfxContext) {
				wglMakeCurrent(_gfxContext.hdc, *_gfxContext.hglrcP);
			}

			~CurrentGfxContext() {
				wglMakeCurrent(nullptr, nullptr);
			}
		};

	public:
		~WinGlUtils();

		// called by a wpf window when it's created or its resolution has changed
		auto CreateGfxContext(HWND _hWnd, GLsizei _viewportW, GLsizei _viewportH) -> Status;

		// called by a wpf window when it's about to close
		void DeleteGfxContext(HWND _hWnd);


		auto InitShader(HWND _hWnd, 
			const char* _vertexShaderSource, const char* _fragmentShaderSource) -> Result<GLuint>;

		auto InitMaterial(HWND _hWnd,
			GLuint _shaderId, const int _framebufferW, const int _framebufferH,
			const GLUtils::TextureIds& _textureIds, const GLUtils::ShaderParams& _paramParams,
			const int _framebufferTextureFilter = GL_NEAREST)
				-> dev::Result<GLUtils::MaterialId>;

		auto InitTexture(HWND _hWnd, GLsizei _w, GLsizei _h, 
			GLUtils::Texture::Format _format, const GLint _textureFilter = GL_NEAREST)
				-> Result<GLuint>;

		void UpdateTexture(HWND _hWnd, const int _texureId, const uint8_t* _memP);

		auto Draw(HWND _hWnd, const GLUtils::MaterialId _renderDataId) const -> dev::ErrCode;
		auto GetFramebufferTexture(HWND _hWnd, const int _materialId) const -> GLuint;
	};
}
#endif // end WPF