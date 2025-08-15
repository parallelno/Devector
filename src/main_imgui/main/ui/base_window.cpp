#include "ui/base_window.h"
#include "utils/imgui_utils.h"

dev::BaseWindow::BaseWindow(const std::string& _name,
	const int _defaultW, const int _defaultH,
	dev::Scheduler& _scheduler,
	bool& _visible,
	const  float* const _dpiScaleP)
	:
	m_name(_name), m_defaultW(_defaultW), m_defaultH(_defaultH),
	m_visible(_visible), m_dpiScaleP(_dpiScaleP)
{
	_scheduler.AddSignal(
		dev::Scheduler::Receiver(
			dev::Scheduler::Signals::UI_DRAW,
			std::bind(&dev::BaseWindow::Draw, this, std::placeholders::_1)));
};


void dev::BaseWindow::Draw(const dev::Scheduler::Signals _signals)
{
	if (!m_visible) return;

	if (!m_default_pos_set){
		SetWindowDefaultPosSize();
		m_default_pos_set = true;
	}
}

void dev::BaseWindow::SetWindowDefaultPosSize()
{
	auto windowPos = ImGui::GetWindowPos();
	ImGui::SetNextWindowPos(ImVec2(windowPos.x, windowPos.y), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2((float)m_defaultW, (float)m_defaultH), ImGuiCond_FirstUseEver);
}
