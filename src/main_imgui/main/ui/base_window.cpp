#include "ui/base_window.h"
#include "utils/imgui_utils.h"

dev::BaseWindow::BaseWindow(const std::string& _name,
	const int _defaultW, const int _defaultH,
	dev::Scheduler& _scheduler,
	bool& _visible,
	const  float* const _dpiScaleP,
	ImGuiWindowFlags _flags)
	:
	m_name(_name), m_defaultW(_defaultW), m_defaultH(_defaultH),
	m_visible(_visible), m_dpiScaleP(_dpiScaleP),
	m_flags(_flags)
{
	_scheduler.AddSignal(
		dev::Scheduler::Receiver(
			dev::Scheduler::Signals::UI_DRAW,
			std::bind(&dev::BaseWindow::Update, this, std::placeholders::_1)));
};


void dev::BaseWindow::Update(const dev::Scheduler::Signals _signals)
{
	if (!m_visible) return;

	if (!m_default_pos_set){
		SetWindowDefaultPosSize();
		m_default_pos_set = true;
	}

	if (ImGui::Begin(m_name.c_str(), &m_visible, m_flags))
	{
		Draw(_signals);
	}
	ImGui::End();
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