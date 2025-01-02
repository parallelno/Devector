#pragma once

#include <map>

#include "utils/imgui_utils.h"
#include "utils/consts.h"
#include "ui/base_window.h"
#include "core/debugger.h"

namespace dev
{
	class BreakpointsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 600;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Hardware& m_hardware;
		ReqUI& m_reqUI;
		Breakpoints::BpMap m_breakpoints;
		size_t m_updates = 0; // stores the updates to prevent extra Debug data fetching

		void DrawTable();
		void DrawPopup(ReqPopup& _reqPopup, const Breakpoints::BpMap& _pbs, int _addr);
		void CheckIfItemClicked(const ImVec2& _rowMin, bool& _showItemContextMenu,
			const int _addr, int& _editedBreakpointAddr, ReqPopup& _reqPopup);
		void UpdateBreakpoints();

	public:
		BreakpointsWindow(Hardware& _hardware,
			const float* const _dpiScaleP, ReqUI& _reqUI);
		void Update(bool& _visible, const bool _isRunning);
		void DrawProperty(const std::string& _name, const ImVec2& _aligment = { 0.0f, 0.5f });
	};

};