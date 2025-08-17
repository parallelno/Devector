#pragma once

#include "utils/imgui_utils.h"
#include "utils/types.h"
#include "utils/consts.h"
#include "ui/base_window.h"
#include "core/debugger.h"
#include "scheduler.h"

namespace dev
{
	class WatchpointsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 600;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Hardware& m_hardware;
		ReqUI& m_reqUI;
		Watchpoints::WpMap m_watchpoints;

		void CheckIfItemClicked(const ImVec2& _rowMin, bool& _showItemContextMenu,
			const int _id, int& _editedWatchpointId, ReqPopup& _reqPopup);
		void UpdateWatchpoints(const dev::Scheduler::Signals _signals);

		void Draw(const dev::Scheduler::Signals _signals) override;
		void DrawPopup(ReqPopup& _reqPopup, const Watchpoints::WpMap& _wps, int _id = -1);
		void DrawProperty(
			const std::string& _name,
			const ImVec2& _aligment = { 0.0f, 0.5f });

	public:
		WatchpointsWindow(Hardware& _hardware,
			dev::Scheduler& _scheduler,
			bool& _visible, const float* const _dpiScaleP, ReqUI& _reqUI);
	};

};