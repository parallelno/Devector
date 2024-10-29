#include "ui/base_window.h"
#include "utils/imgui_utils.h"

void dev::BaseWindow::Update()
{
	SetWindowDefaultPosSize();
}

void dev::BaseWindow::SetWindowDefaultPosSize()
{
	auto windowPos = ImGui::GetWindowPos();
	ImGui::SetNextWindowPos(ImVec2(windowPos.x, windowPos.y), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2((float)m_defaultW, (float)m_defaultH), ImGuiCond_FirstUseEver);
}
