#pragma once
#ifndef DEV_DISPLAY_WINDOW_H
#define DEV_DISPLAY_WINDOW_H

#include <vector>

#include "../Devector/Types.h"
#include "Utils/Globals.h"

#include "Ui/BaseWindow.h"
#include "Utils/Result.h"
#include "../Devector/Hardware.h"

#include "../Utils/GLUtils.h"

namespace dev
{
	class DisplayWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 768;
		static constexpr float WINDOW_ASPECT = 3.0f / 4.0f;
		static constexpr int DEFAULT_WINDOW_H = static_cast<int>(DEFAULT_WINDOW_W * WINDOW_ASPECT);

		static constexpr int FRAME_BUFFER_W = 768;
		static constexpr int FRAME_BUFFER_H = 256;



		const std::string m_name = "Display";

		GLuint1 m_frameTextureId = 0;
		Hardware& m_hardware;
		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;
		bool m_isHovered;
		GLUtils::Vec4 m_shaderData_scrollVert = {255.0f, 0, 0, 0};

		GLUtils& m_glUtils;
		int m_renderDataIdx = -1;

		void DrawDisplay();
		void CreateTexture(const bool _vsync);
		void UpdateData(const bool _isRunning);

	public:
		DisplayWindow(Hardware& _hardware, const float* const _fontSizeP, const float* const _dpiScaleP, GLUtils& _glUtils);
		void Update();
		bool IsHovered();
	};

};

#endif // !DEV_DISPLAY_WINDOW_H