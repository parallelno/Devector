#pragma once
#include "imgui.h"

#include "Utils/ImGuiUtils.h"
#include "Ui/BaseWindow.h"
#include "Core/Hardware.h"

namespace dev
{
	class RecorderWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Hardware& m_hardware;
		Debugger& m_debugger;
		ReqUI& m_reqUI;

		int64_t m_ccLast = -1; // to force the first stats update


		void UpdateData(const bool _isRunning);

	public:
		RecorderWindow(Hardware& _hardware, Debugger& _debugger, 
			const float* const _fontSizeP, const float* const _dpiScaleP,
			ReqUI& _reqUI);
		void Update(bool& _visible);
		void Draw(const bool _isRunning);
	};
};