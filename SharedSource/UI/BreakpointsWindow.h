#pragma once
#ifndef DEV_BREAKPOINTS_WINDOW_H
#define DEV_BREAKPOINTS_WINDOW_H

#include "Utils/Globals.h"
#include "Utils/ImGuiUtils.h"
#include "Ui/BaseWindow.h"
#include "../Devector/Hardware.h"

namespace dev
{
	class BreakpointsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 600;
		static constexpr int DEFAULT_WINDOW_H = 300;
		// colors
		static constexpr ImVec4 COLOR_WARNING = dev::IM_VEC4(0xFF2020FF);

		Hardware& m_hardware;
		bool& m_reqDisasmUpdate;

		void DrawTable();
		void DrawContextMenu(const char* _itemID);

	public:
		BreakpointsWindow(Hardware& _hardware,
			const float* const _fontSizeP, const float* const _dpiScaleP, bool& _reqDisasmUpdate);

		void Update();

		void DrawProperty(const std::string& _name, const ImVec2& _aligment = { 0.0f, 0.5f });
	};

};

#endif // !DEV_BREAKPOINTS_WINDOW_H