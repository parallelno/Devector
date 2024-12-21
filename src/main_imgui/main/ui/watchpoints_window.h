#pragma once

#include "utils/imgui_utils.h"
#include "utils/types.h"
#include "utils/consts.h"
#include "ui/base_window.h"
#include "core/debugger.h"

namespace dev
{
	class WatchpointsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 600;
		static constexpr int DEFAULT_WINDOW_H = 300;
		static constexpr ImVec4 COLOR_WARNING = dev::IM_VEC4(0xFF2020FF);

		Hardware& m_hardware;
		ReqUI& m_reqUI;
		Watchpoints::WpMap m_watchpoints;
		size_t m_updates = 0; // stores the watchpoint data updates to prevent extra Debug data fetching

		void DrawTable();
		void DrawPopup(ReqPopup& _reqPopup, const Watchpoints::WpMap& _wps, int _id = -1);
		void CheckIfItemClicked(const ImVec2& _rowMin, bool& _showItemContextMenu,
			const int _id, int& _editedWatchpointId, ReqPopup& _reqPopup);
		void UpdateWatchpoints();

	public:
		WatchpointsWindow(Hardware& _hardware,
			const float* const _dpiScaleP, ReqUI& _reqUI);
		void Update(bool& _visible, const bool _isRunning);
		void DrawProperty(const std::string& _name, const ImVec2& _aligment = { 0.0f, 0.5f });
	};

};