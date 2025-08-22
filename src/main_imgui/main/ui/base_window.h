#pragma once

#include <string>

#include "utils/imgui_utils.h"
#include "scheduler.h"

namespace dev
{
	class BaseWindow
	{
		int m_defaultW;
		int m_defaultH;

	protected:
		ImVec2 buttonSize = { 65.0f, 25.0f };

		enum class Type {
			Window,
			Popup
		};

		const float* const m_dpiScaleP = nullptr;
		// Use nullptr for popups
		bool* m_visibleP = nullptr;
		bool m_default_pos_set = false;
		ImGuiWindowFlags m_flags = 0;
		Type m_type = Type::Window;

		enum class WinPosPreset {
			NONE,
			CENTER
		};
		dev::Scheduler& m_scheduler;

		void SetWindowPos(const WinPosPreset _preset);

	public:
		const std::string m_name;

		BaseWindow(const std::string& _name,
			const int _defaultW, const int _defaultH,
			dev::Scheduler& _scheduler,
			bool* _visibleP = nullptr,
			const  float* const _dpiScaleP,
			ImGuiWindowFlags _flags = ImGuiWindowFlags_NoCollapse,
			BaseWindow::Type _type = BaseWindow::Type::Window);

		void CallbackUpdate(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);
		virtual void Draw(
			const dev::Signals _signals, dev::Scheduler::SignalData _data) = 0;
		void SetWindowDefaultPosSize();
	};
}