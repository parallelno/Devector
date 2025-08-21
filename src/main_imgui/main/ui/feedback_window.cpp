#include <format>
#include "ui/feedback_window.h"
#include "utils/str_utils.h"

dev::FeedbackWindow::FeedbackWindow(
	dev::Scheduler& _scheduler, bool& _visible,
	const float* const _dpiScaleP)
	:
	BaseWindow("Send Feedback", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visible, _dpiScaleP,
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse
		)
{
	m_userFeedback = m_defaultFeedback;
}


void dev::FeedbackWindow::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	SetWindowPos(WinPosPreset::CENTER);

	DrawContext();
	DrawConfirmation();
}

void dev::FeedbackWindow::DrawContext()
{
	dev::DrawHelpMarker("Please, use english alphabet only, \nbecause the current font does not support \ncyrillic in the current version.");

	ImGui::InputTextMultiline("##feedback", m_userFeedback.data(), FEEDBACK_LEN_MAX, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_AllowTabInput);
	ImGui::Separator();

	ImGui::Dummy({ 1, 5 });
	if (ImGui::Button("Send Feedback"))
	{
		std::string info = __DATE__;
		std::copy(info.begin(), info.end() - 1, m_sysInfo.data());
		ImGui::OpenPopup(POPUP_CONFIRMATION_NAME);
	}
	ImGui::Dummy({ 1, 5 });
}

// Popup window. Display the full feedback message and ask for confirmation to send it
void dev::FeedbackWindow::DrawConfirmation()
{
	ImVec2 center = ImGui::GetMainViewport()->GetCenter(); 	// Always center this window when appearing
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(POPUP_CONFIRMATION_NAME, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Full message:");
		ImGui::Separator();
		ImGui::InputTextMultiline("##feedbackconf1", m_userFeedback.data(), FEEDBACK_LEN_MAX, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 10) );
		ImGui::InputTextMultiline("##feedbackconf2", m_sysInfo.data(), FEEDBACK_LEN_MAX, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 6));

		ImGui::Separator();
		ImGui::Dummy({ DEFAULT_WINDOW_W, 5 });

		if (ImGui::Button("Confirm", ImVec2(120, 0)))
		{
			m_userFeedback = m_defaultFeedback;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		ImGui::Dummy({ 10, 1 });
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::Dummy({ 1, 5 });

		ImGui::EndPopup();
	}
}