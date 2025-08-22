#include "ui/base_window.h"
#include "utils/imgui_utils.h"

dev::BaseWindow::BaseWindow(const std::string& _name,
	const int _defaultW, const int _defaultH,
	dev::Scheduler& _scheduler,
	bool* _visibleP,
	const  float* const _dpiScaleP,
	ImGuiWindowFlags _flags,
	BaseWindow::Type _type)
	:
	m_name(_name), m_defaultW(_defaultW), m_defaultH(_defaultH),
	m_visibleP(_visibleP), m_dpiScaleP(_dpiScaleP),
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
	if (!m_default_pos_set){
		SetWindowDefaultPosSize();
		m_default_pos_set = true;
	}

	switch (m_type){
		case Type::Window:{
			if (ImGui::Begin(m_name.c_str(), m_visibleP, m_flags))
			{
				Draw(_signals, std::nullopt);
			}
			ImGui::End();
			break;
		}
		case Type::Popup:{
			ImVec2 winPos = ImGui::GetMousePos();
			ImGui::SetNextWindowPos(
				winPos, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

			if (ImGui::BeginPopupModal(m_name.c_str(), NULL, m_flags))
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

void dev::BaseWindow::SetWindowDefaultPosSize()
{
	auto windowPos = ImGui::GetWindowPos();
	ImGui::SetNextWindowPos(ImVec2(windowPos.x, windowPos.y), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2((float)m_defaultW, (float)m_defaultH), ImGuiCond_FirstUseEver);
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