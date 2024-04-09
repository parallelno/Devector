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
		static constexpr int FRAME_BUFFER_W = 1024;
		static constexpr int FRAME_BUFFER_H = 512;

		static constexpr int DEFAULT_WINDOW_W = 1024;
		static constexpr int DEFAULT_WINDOW_H = 512;

		static constexpr float SCALE_MAX = 3.0f;
		static constexpr float SCALE_MIN = 0.3f;
		static constexpr float SCALE_INC = 0.2f;

		static constexpr int RAM_TEXTURES = Memory::GLOBAL_MEMORY_LEN / Memory::MEM_64K;
		static constexpr int RAM_TEXTURE_W = 256;
		static constexpr int RAM_TEXTURE_H = Memory::MEMORY_MAIN_LEN / 256;

		Hardware& m_hardware;
		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;
		float m_scale = 1.0f;

		GLUtils& m_glUtils;
		GLUtils::Vec4 m_globalColorBg = { 0.2f, 0.2f, 0.2f, 1.0f };
		GLUtils::Vec4 m_globalColorFg = { 1.0f, 1.0f, 1.0f, 1.0f };
		int m_renderDataIdx = -1;

		void DrawDisplay();
		void UpdateData(const bool _isRunning);
		void Init();
		void ScaleView();

	public:
		RamViewWindow(Hardware& _hardware, const float* const _fontSizeP, const float* const _dpiScaleP, GLUtils& _glUtils);
		void Update();
	};

};

#endif // !DEV_RAM_VIEW_WINDOW_H