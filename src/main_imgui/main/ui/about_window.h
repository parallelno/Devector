#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "scheduler.h"

namespace dev
{
	class AboutWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		const std::string compilation_date = __DATE__;

		void DrawContext();

	public:
		AboutWindow(dev::Scheduler& _scheduler,
			bool& _visible,
			const float* const _dpiScaleP);
		void Draw(const dev::Scheduler::Signals _signals) override;
	};
};