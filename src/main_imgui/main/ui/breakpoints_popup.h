#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "scheduler.h"

namespace dev
{
	class BreakpointsPopup : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		dev::Hardware& m_hardware;
		dev::Debugger& m_debugger;

		dev::Signals m_signal = dev::Signals::NONE;
		bool m_isActive = true;
		Breakpoint::MemPages m_memPages = Breakpoint::MAPPING_PAGES_ALL;
		int m_addr = 0;
		Addr m_addrOld = 0xFF;
		int m_val = 0;
		bool m_isAutoDel = false;
		int m_selectedOp = 0;
		int m_selectedCond = 0;
		std::string m_comment = "";


		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;

		void CallbackAdd(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);
		void CallbackEdit(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);

	public:
		BreakpointsPopup(
			dev::Hardware& _hardware, dev::Debugger& _debugger,
			dev::Scheduler& _scheduler,
			bool* _visibleP,
			const float* const _dpiScaleP);
	};
};