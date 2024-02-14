#pragma once
#ifndef DEV_HARDWARE_STATS_WINDOW_H
#define DEV_HARDWARE_STATS_WINDOW_H

#include <mutex>

#include "Utils/Globals.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/BaseWindow.h"

namespace dev
{
	class HardwareStatsWindow : public BaseWindow
	{	
		void DrawGlobalStats();

	public:
		HardwareStatsWindow();
		
		void Update();

	};

};

#endif // !DEV_HARDWARE_STATS_WINDOW_H