#include <string>
#include "scheduler.h"

#pragma once

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

	public:
		const std::string m_name;

		BaseWindow(const std::string& _name,
			const int _defaultW, const int _defaultH,
			dev::Scheduler& _scheduler,
			bool& _visible,
			const  float* const _dpiScaleP);

		virtual void Draw(const dev::Scheduler::Signals _signals);
		void SetWindowDefaultPosSize();
	};
}