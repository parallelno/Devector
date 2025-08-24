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

		void DrawContext();

	public:
		AboutWindow(dev::Scheduler& _scheduler,
			bool* _visibleP,
			const float* const _dpiScaleP);
		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;
	};
};