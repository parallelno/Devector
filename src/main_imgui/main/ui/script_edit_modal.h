#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "scheduler.h"

namespace dev
{
	class ScriptEditModal : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;
		static constexpr int CODE_LEN_MAX = 10000;

		Hardware& m_hardware;
		Debugger& m_debugger;

		bool m_enterPressed = false;
		bool m_setFocus = false;
		Script m_script;
		char m_code[CODE_LEN_MAX];

		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;

		void CallbackAdd(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);
		void CallbackEdit(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);

	public:
		ScriptEditModal(
			Hardware& _hardware, Debugger& _debugger,
			dev::Scheduler& _scheduler,
			bool* _visibleP,
			const float* const _dpiScaleP);
	};
};