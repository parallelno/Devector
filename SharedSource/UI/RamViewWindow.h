#pragma once
#ifndef DEV_RAM_VIEW_WINDOW_H
#define DEV_RAM_VIEW_WINDOW_H

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <vector>

#include "../Devector/Types.h"
#include "Utils/Globals.h"
#include "Ui/BaseWindow.h"
#include "Utils/Result.h"
#include "../Devector/Hardware.h"

#include "../Utils/GLUtils.h"

namespace dev
{
	class RamViewWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 1024;
		static constexpr int DEFAULT_WINDOW_H = 512;

		static constexpr float SCALE_MAX = 3.0f;
		static constexpr float SCALE_MIN = 0.3f;
		static constexpr float SCALE_INC = 0.2f;

		Hardware& m_hardware;
		GLUtils m_glUtils;
		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;
		float m_scale = 1.0f;

		void DrawDisplay();
		void UpdateData(const bool _isRunning);
		void Init();
		void ScaleView();

	public:
		RamViewWindow(Hardware& _hardware, const float* const _fontSizeP, const float* const _dpiScaleP);
		void Update();
	};

};

#endif // !DEV_RAM_VIEW_WINDOW_H