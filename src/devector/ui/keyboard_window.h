#pragma once

#include "core/hardware.h"
#include "utils/imgui_utils.h"
#include "ui/base_window.h"

namespace dev
{
	class KeyboardWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 600;
		static constexpr int DEFAULT_WINDOW_H = 250;

		Hardware& m_hardware;
		ReqUI& m_reqUI;
		std::string m_keyboardImgPath;
		bool m_inited = false;

		//void UpdateData(const bool _isRunning);

	public:
		KeyboardWindow(Hardware& _hardware, 
			const float* const _dpiScaleP, ReqUI& _reqUI, const std::string& _pathImgVector);
		void Update(bool& _visible);
		void Draw(const bool _isRunning);
	};
};