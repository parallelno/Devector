#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "scheduler.h"

namespace dev
{
	class CommentEditWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		dev::Hardware& m_hardware;
		dev::Debugger& m_debugger;

		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;

		void CallbackAdd(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);
		void CallbackEdit(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);

	public:
		CommentEditWindow(
			dev::Hardware& _hardware, dev::Debugger& _debugger,
			dev::Scheduler& _scheduler,
			bool* _visibleP,
			const float* const _dpiScaleP);
	};
};