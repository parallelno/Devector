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

namespace dev
{
	class RamViewWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 256;
		static constexpr int DEFAULT_WINDOW_H = 256;

		struct ShaderData {
			GLuint1 shaderProgram = 0;
			GLuint1 frameTextureId = 0;
		};
		ShaderData m_shaderData;
		Hardware& m_hardware;
		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;

		void DrawDisplay();
		void CreateTexture(const bool _vsync);
		void UpdateData(const bool _isRunning);
		void Init();

	public:
		RamViewWindow(Hardware& _hardware, const float* const _fontSizeP, const float* const _dpiScaleP);
		void Update();
	};

};

#endif // !DEV_RAM_VIEW_WINDOW_H