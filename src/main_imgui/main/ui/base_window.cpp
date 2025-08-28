#include "ui/base_window.h"
#include "utils/imgui_utils.h"

dev::BaseWindow::BaseWindow(const std::string& _name,
	const int _defaultW, const int _defaultH,
	dev::Scheduler& _scheduler,
	bool* _visibleP,
	ImGuiWindowFlags _flags,
	BaseWindow::Type _type)
	:
	m_name(_name),
	m_defaultW(_defaultW),
	m_defaultH(_defaultH),
	m_visibleP(_visibleP),
	m_flags(_flags), m_scheduler(_scheduler), m_type(_type)
{
	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::UI_DRAW,
			std::bind(&dev::BaseWindow::CallbackUpdate, this,
				std::placeholders::_1, std::placeholders::_2),
			m_visibleP));
};


void dev::BaseWindow::CallbackUpdate(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	SetWindowDefaultPosSize(ImGuiCond_FirstUseEver);

	auto scale = ImGui::GetWindowDpiScale();
	m_buttonSize.x = scale * 65.0f;
	m_buttonSize.y = scale * 25.0f;

	switch (m_type){
		case Type::Window:
		{
			if (ImGui::Begin(m_name.c_str(), m_visibleP, m_flags))
			{
				Draw(_signals, std::nullopt);
			}
			ImGui::End();
			break;
		}
		case Type::Modal:
		{
			SetPopUpWindowPos();

			if (ImGui::BeginPopupModal(m_name.c_str(), NULL, m_flags))
			{
				Draw(_signals, std::nullopt);
				ImGui::EndPopup();
			}
			break;
		}
		case Type::Popup:
		{
			SetPopUpWindowPos();

			if (ImGui::BeginPopup(m_name.c_str()))
			{
				Draw(_signals, std::nullopt);
				ImGui::EndPopup();
			}
			break;
		}
		default:
			break;
	}
}

void dev::BaseWindow::SetWindowDefaultPosSize(ImGuiCond cond)
{
	auto windowPos = ImGui::GetWindowPos();
	ImGui::SetNextWindowPos(ImVec2(windowPos.x, windowPos.y), cond);
	ImGui::SetNextWindowSize(ImVec2(
		(float)m_defaultW * ImGui::GetWindowDpiScale(),
		(float)m_defaultH * ImGui::GetWindowDpiScale()),
		cond);
}

void dev::BaseWindow::SetPopUpWindowPos()
{
	ImVec2 winPos = ImGui::GetMousePos();
	ImGui::SetNextWindowPos(
		winPos, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
}

void dev::BaseWindow::SetWindowPos(const WinPosPreset _preset)
{
	switch (_preset){
		case WinPosPreset::CENTER:
		{
			// Always center this window when appearing
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(
				center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			break;
		}
		default:
			break;
	}

}