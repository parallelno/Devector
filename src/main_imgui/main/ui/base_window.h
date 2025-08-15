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
		const float* const m_dpiScaleP = nullptr;
		bool& m_visible;
		bool m_default_pos_set = false;
		ImGuiWindowFlags m_flags = 0;

		enum class WinPosPreset {
			NONE,
			CENTER
		};

		void SetWindowPos(const WinPosPreset _preset);

	public:
		const std::string m_name;

		BaseWindow(const std::string& _name,
			const int _defaultW, const int _defaultH,
			dev::Scheduler& _scheduler,
			bool& _visible,
			const  float* const _dpiScaleP,
			ImGuiWindowFlags _flags = ImGuiWindowFlags_NoCollapse);

		void Update(const dev::Scheduler::Signals _signals);
		virtual void Draw(const dev::Scheduler::Signals _signals) = 0;
		void SetWindowDefaultPosSize();
	};
}