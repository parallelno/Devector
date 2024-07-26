#pragma once

#include <map>
#include "imgui.h"

#include "Utils/ImGuiUtils.h"
#include "Utils/Consts.h"
#include "Ui/BaseWindow.h"
#include "Core/Debugger.h"

namespace dev
{
	class BreakpointsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 600;
		static constexpr int DEFAULT_WINDOW_H = 300;
		static constexpr ImVec4 COLOR_WARNING = dev::IM_VEC4(0xFF2020FF);

		Hardware& m_hardware;
		Debugger& m_debugger;
		ReqUI& m_reqUI;
		size_t m_bpUpdates = 0; // stores the updates to prevent extra Debug data fetching
		Breakpoints::BpMap m_breakpoints;

		void DrawTable();
		void DrawPopup(ReqPopup& _reqPopup, const Breakpoints::BpMap& _pbs, int _addr);
		void CheckIfItemClicked(const ImVec2& _rowMin, bool& _showItemContextMenu,
			const int _addr, int& _editedBreakpointAddr, ReqPopup& _reqPopup);
		void UpdateBreakpoints();

	public:
		BreakpointsWindow(Hardware& _hardware, Debugger& m_debugger,
			const float* const _fontSizeP, const float* const _dpiScaleP, ReqUI& _reqUI);
		void Update(bool& _visible);
		void DrawProperty(const std::string& _name, const ImVec2& _aligment = { 0.0f, 0.5f });
	};

};