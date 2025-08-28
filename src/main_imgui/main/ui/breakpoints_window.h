#pragma once

#include <map>

#include "utils/imgui_utils.h"
#include "utils/consts.h"
#include "ui/base_window.h"
#include "core/debugger.h"
#include "scheduler.h"

namespace dev
{
	class BreakpointsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 600;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Hardware& m_hardware;
		Breakpoints::BpMap m_breakpoints;

		void CheckIfItemClicked(const ImVec2& _rowMin, bool& _showItemContextMenu,
			const int _addr, int& _editedBreakpointAddr);
		void CallbackUpdateBreakpoints(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data);

		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;
		void DrawProperty(const std::string& _name,
						const ImVec2& _aligment = { 0.0f, 0.5f });

	public:
		BreakpointsWindow(Hardware& _hardware,
			dev::Scheduler& _scheduler,
			bool* _visibleP);
	};

};