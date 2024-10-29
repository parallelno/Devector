#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "core/hardware.h"

namespace dev
{
	class RecorderWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Hardware& m_hardware;
		Debugger& m_debugger;
		ReqUI& m_reqUI;

		int64_t m_ccLast = -1; // to force the first stats update
		int m_stateRecorded = 0;
		int m_stateCurrent = 0;

		void UpdateData(const bool _isRunning);

	public:
		RecorderWindow(Hardware& _hardware, Debugger& _debugger, 
			const float* const _dpiScaleP,
			ReqUI& _reqUI);
		void Update(bool& _visible);
		void Draw(const bool _isRunning);
	};
};