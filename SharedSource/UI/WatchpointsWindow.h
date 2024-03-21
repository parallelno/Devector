#pragma once
#ifndef DEV_WATCHPOINTS_WINDOW_H
#define DEV_WATCHPOINTS_WINDOW_H

#include "Utils/Globals.h"
#include "Ui/BaseWindow.h"
#include "../Devector/Debugger.h"

namespace dev
{
	class WatchpointsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 600;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Debugger& m_debugger;

		void DrawTable();

	public:
		WatchpointsWindow(Debugger& _debugger,
			const float* const _fontSizeP, const float* const _dpiScaleP);

		void Update();

	};

};

#endif // !DEV_WATCHPOINTS_WINDOW_H