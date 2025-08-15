#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "core/hardware.h"
#include "scheduler.h"

namespace dev
{
	class SearchWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Hardware& m_hardware;
		Debugger& m_debugger;
		ReqUI& m_reqUI;

		int64_t m_ccLast = -1; // to force the first stats update

		bool m_searchEnabled = false;
		int m_searchStartAddr = 0x0;
		int m_searchEndAddr = 0xFFFF;
		int m_searchVal = 0x0;

		std::vector<GlobalAddr> m_searchResults;

		void UpdateData(const bool _isRunning);

	public:
		SearchWindow(Hardware& _hardware, Debugger& _debugger,
			dev::Scheduler& _scheduler,
			bool& _visible, const float* const _dpiScaleP,
			ReqUI& _reqUI);
		void Draw(const dev::Scheduler::Signals _signals) override;
		void DrawContext(const bool _isRunning);
	};
};