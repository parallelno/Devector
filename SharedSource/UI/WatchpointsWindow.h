#pragma once
#ifndef DEV_WATCHPOINTS_WINDOW_H
#define DEV_WATCHPOINTS_WINDOW_H

#include "imgui.h"

#include "Utils/ImGuiUtils.h"
#include "Utils/Globals.h"
#include "Ui/BaseWindow.h"
#include "../Devector/Debugger.h"

namespace dev
{
	class WatchpointsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 600;
		static constexpr int DEFAULT_WINDOW_H = 300;
		static constexpr ImVec4 COLOR_WARNING = dev::IM_VEC4(0xFF2020FF);

		Debugger& m_debugger;
		int& m_reqDisasmUpdate;

		void DrawTable();
		void DrawPopup(ReqPopup& _reqPopup, const Debugger::Watchpoints& _wps, int _id = -1);

	public:
		WatchpointsWindow(Debugger& _debugger,
			const float* const _fontSizeP, const float* const _dpiScaleP, int& _reqDisasmUpdate);

		void Update();

		void DrawProperty(const std::string& _name, const ImVec2& _aligment = { 0.0f, 0.5f });
		void DrawProperty2Access(const char* _name, int* _access, const char* _hint = "");
		void DrawProperty2Type(const char* _name, int* _type, const char* _hint = "");
	};

};

#endif // !DEV_WATCHPOINTS_WINDOW_H