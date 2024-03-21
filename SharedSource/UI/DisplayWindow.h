#pragma once
#ifndef DEV_DISPLAY_WINDOW_H
#define DEV_DISPLAY_WINDOW_H

#include <vector>

#include "../Devector/Types.h"
#include "Utils/Globals.h"

#include "Ui/BaseWindow.h"
#include "Utils/Result.h"
#include "../Devector/Hardware.h"

namespace dev
{
	class DisplayWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 768;
		static constexpr int DEFAULT_WINDOW_H = 312;

		GLuint1 m_frameTextureId = 0;
		Hardware& m_hardware;
		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;

		void DrawDisplay();
		void CreateTexture(const bool _vsync);
		void UpdateData(const bool _isRunning);

	public:
		DisplayWindow(Hardware& _hardware, const float* const _fontSizeP, const float* const _dpiScaleP);
		void Update();
	};

};

#endif // !DEV_DISPLAY_WINDOW_H