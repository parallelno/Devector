#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "scheduler.h"

namespace dev
{
	class WatchpointsPopup : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 420;
		static constexpr int DEFAULT_WINDOW_H = 320;

		dev::Hardware& m_hardware;
		dev::Debugger& m_debugger;

		dev::Signals m_signal = dev::Signals::NONE;
		bool m_isActive = true;
		int m_oldId = -1;
		int m_globalAddr = 0xFF;
		int m_access = static_cast<int>(Watchpoint::Access::RW);
		int m_cond = static_cast<int>(dev::Condition::ANY);
		int m_val = 0;
		int m_type = static_cast<int>(Watchpoint::Type::LEN);
		int m_len = 1;
		std::string m_comment = "";

		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;

		void CallbackAdd(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);
		void CallbackEdit(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);

	public:
		WatchpointsPopup(
			dev::Hardware& _hardware, dev::Debugger& _debugger,
			dev::Scheduler& _scheduler,
			bool* _visibleP = nullptr);
	};
};