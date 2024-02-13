#pragma once
#ifndef HARDWARE_STATS_WINDOW_H
#define HARDWARE_STATS_WINDOW_H

#include "utils/globals.h"
#include "ui/imGuiUtils.h"
#include "ui/baseWindow.h"
#include <mutex>

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

#endif // !HARDWARE_STATS_WINDOW_H