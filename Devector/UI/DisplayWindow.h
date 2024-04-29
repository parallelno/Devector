#pragma once
#ifndef DEV_DISPLAY_WINDOW_H
#define DEV_DISPLAY_WINDOW_H

#include <vector>

#include "Utils/Types.h"
#include "Utils/Consts.h"
#include "Ui/BaseWindow.h"
#include "Utils/Result.h"
#include "Core/Hardware.h"
#include "Utils/GLUtils.h"

namespace dev
{
	class DisplayWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 768;
		static constexpr float WINDOW_ASPECT = 3.0f / 4.0f;
		static constexpr int DEFAULT_WINDOW_H = static_cast<int>(DEFAULT_WINDOW_W * WINDOW_ASPECT);
		static constexpr int FRAME_W = 512 + 32 * 2;
		static constexpr int FRAME_H = 256 + 16 * 2;

		const std::string m_name = "Display";

		GLuint1 m_frameTextureId = 0;
		Hardware& m_hardware;
		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;
		bool m_isHovered;
		
		GLUtils& m_glUtils;
		GLUtils::Vec4 m_shaderData_scrollVert = {255.0f, 0, 0, 0};
		GLUtils::Vec4 m_shaderData_bordL_bordB_visBord = {
						(float)Display::BORDER_LEFT, 
						static_cast<float>(Display::SCAN_VBLANK_BOTTOM), 
						static_cast<float>(Display::BORDER_VISIBLE), 
						0.0f};
		GLuint1 m_vramShaderId = -1;
		GLUtils::MaterialId m_vramMatId;
		GLuint1 m_vramTexId = -1;
		bool m_isGLInited = false;

		void DrawDisplay();
		void CreateTexture(const bool _vsync);
		void UpdateData(const bool _isRunning);
		bool Init();

	public:
		DisplayWindow(Hardware& _hardware, const float* const _fontSizeP, const float* const _dpiScaleP, GLUtils& _glUtils);
		void Update();
		bool IsHovered() const;
	};

};

#endif // !DEV_DISPLAY_WINDOW_H