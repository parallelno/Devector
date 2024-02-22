#pragma once
#ifndef DEV_HARDWARE_STATS_WINDOW_H
#define DEV_HARDWARE_STATS_WINDOW_H

#include "Utils/Globals.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/BaseWindow.h"

namespace dev
{
	class HardwareStatsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		void DrawGlobalStats();

	public:
		HardwareStatsWindow();
		
		void Update();

	};

};

#endif // !DEV_HARDWARE_STATS_WINDOW_H