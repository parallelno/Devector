#pragma once
#ifndef DEV_HARDWARE_STATS_WINDOW_H
#define DEV_HARDWARE_STATS_WINDOW_H

#include "Utils/Globals.h"
#include "Utils/ImGuiUtils.h"
#include "Ui/BaseWindow.h"
#include "../Devector/Hardware.h"

namespace dev
{
	class HardwareStatsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		int64_t m_ccLast = 0;
		int64_t m_ccLastRun = 0;

		Hardware& m_hardware;

		void DrawStats();
		void DrawRegs();
		void DrawFlags();
		void DrawStack();
		void DrawHardware();

	public:
		HardwareStatsWindow(Hardware& _hardware,
			const float* const _fontSizeP, const float* const _dpiScaleP);
		
		void Update();

	};

};

#endif // !DEV_HARDWARE_STATS_WINDOW_H