#pragma once
#ifndef DEV_HARDWARE_STATS_WINDOW_H
#define DEV_HARDWARE_STATS_WINDOW_H

#include "Utils/Globals.h"
#include "Utils/ImGuiUtils.h"
#include "Ui/BaseWindow.h"
#include "../Devector/Hardware.h"

namespace dev
{
	class HardwareStatsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		int64_t m_ccLast = 0;

		Hardware& m_hardware;

		void DrawStats();
		void DrawRegs();
		void DrawFlags();
		void DrawStack();
		void DrawHardware();
		void DrawProperty1(const std::string& _name, const std::string& _value, const ImVec2& _aligment = { 1.0f, 0.5f });
		void DrawProperty2(const std::string& _name, const std::string& _value);
		void DrawSeparator2(const std::string& _text);

	public:
		HardwareStatsWindow(Hardware& _hardware,
			const float const* _fontSizeP, const float const* _dpiScaleP);
		
		void Update();

	};

};

#endif // !DEV_HARDWARE_STATS_WINDOW_H