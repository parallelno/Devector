#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "scheduler.h"

namespace dev
{
	class DisasmPopup : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 420;
		static constexpr int DEFAULT_WINDOW_H = 320;

		dev::Hardware& m_hardware;
		dev::Debugger& m_debugger;

		dev::Signals m_signal = dev::Signals::NONE;
		bool m_isActive = true;
		GlobalAddr m_globalAddr = 0;
		bool m_immHovered = false;
		bool m_editMemoryExists = false;

		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;

		void CallbackOpen(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);

	public:
		DisasmPopup(
			dev::Hardware& _hardware, dev::Debugger& _debugger,
			dev::Scheduler& _scheduler,
			bool* _visibleP = nullptr);
	};
};