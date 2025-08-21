#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "scheduler.h"

namespace dev
{
	class ConstEditWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		dev::Hardware& m_hardware;
		dev::Debugger& m_debugger;

		bool m_enterPressed = false;
		bool m_setFocus = false;
		int m_addr = 0;
		int m_oldAddr = 0;
		DebugData::LabelList m_consts;
		int m_selectedItemIdx = 0;
		bool m_editConst = false;


		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;

		void CallbackAdd(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);
		void CallbackEdit(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);

	public:
		ConstEditWindow(
			dev::Hardware& _hardware, dev::Debugger& _debugger,
			dev::Scheduler& _scheduler,
			bool& _visible,
			const float* const _dpiScaleP);
	};
};