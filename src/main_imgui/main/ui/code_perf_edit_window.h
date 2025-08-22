#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "scheduler.h"
#include "core/code_perf.h"

namespace dev
{
	class CodePerfEditWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Hardware& m_hardware;
		Debugger& m_debugger;

		bool m_enterPressed = false;
		bool m_setFocus = false;
		int m_addrStart = 0;
		int m_addrEnd = 0;
		dev::CodePerf m_codePerf;


		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;

		void CallbackAdd(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);
		void CallbackEdit(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);

	public:
		CodePerfEditWindow(
			Hardware& _hardware, Debugger& _debugger,
			dev::Scheduler& _scheduler,
			bool* _visibleP,
			const float* const _dpiScaleP);
	};
};