#pragma once
#include "utils/imgui_utils.h"

#include "ui/base_window.h"
//#include "utils/telegramBot.h"
#include <array>

namespace dev
{
	class FeedbackWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;
		static constexpr int FEEDBACK_LEN_MAX = 1024;
		const char* POPUP_CONFIRMATION_NAME = "Confirmation";

		const std::array<char, FEEDBACK_LEN_MAX> m_defaultFeedback = {
			"Describe the steps to repro the issue:\n"
			"...\n\n"
			"What workaround/solution/suggetion do you have if any?\n"
			"...\n\n"
			"Thank you!\n"
		};
		std::array<char, FEEDBACK_LEN_MAX> m_userFeedback;
		std::array<char, FEEDBACK_LEN_MAX> m_sysInfo;

		//TelegramBot m_telegramBot;

	public:
		FeedbackWindow(const float* const _dpiScaleP);
		void Update(bool& _visible, const bool _isRunning);
		void Draw();
		void DrawConfirmation();
	};
};