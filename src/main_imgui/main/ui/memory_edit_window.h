#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "scheduler.h"
#include "core/memory_edit.h"

namespace dev
{
	class MemoryEditWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Hardware& m_hardware;
		Debugger& m_debugger;

		bool m_enterPressed = false;
		bool m_setFocus = false;
		int m_globalAddr = 0;
		int m_value = 0;
		dev::MemoryEdit m_edit;

		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;

		void CallbackAdd(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);
		void CallbackEdit(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);

	public:
		MemoryEditWindow(
			Hardware& _hardware, Debugger& _debugger,
			dev::Scheduler& _scheduler,
			bool* _visibleP,
			const float* const _dpiScaleP);
	};
};