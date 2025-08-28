#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "core/hardware.h"
#include "scheduler.h"

namespace dev
{
	class RecorderWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Hardware& m_hardware;
		Debugger& m_debugger;

		int m_stateRecorded = 0;
		int m_stateCurrent = 0;

		void CallbackUpdateData(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data);
		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;

	public:
		RecorderWindow(Hardware& _hardware, Debugger& _debugger,
			dev::Scheduler& _scheduler,
			bool* _visibleP);
	};
};