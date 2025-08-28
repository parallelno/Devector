#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "scheduler.h"

namespace dev
{
	class TraceLogPopup : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		dev::Hardware& m_hardware;
		dev::Debugger& m_debugger;

		Addr m_addr = 0;
		std::string m_str = "";

		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;

		void CallbackOpen(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);

	public:
		TraceLogPopup(
			dev::Hardware& _hardware, dev::Debugger& _debugger,
			dev::Scheduler& _scheduler,
			bool* _visibleP = nullptr);
	};
};