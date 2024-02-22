#include "BaseWindow.h"
#include "Utils/ImGuiUtils.h"

void dev::BaseWindow::Close()
{
	m_visible = false;
}

void dev::BaseWindow::Update()
{
	SetWindowDefaultPosSize();
}

void dev::BaseWindow::Open()
{
	m_visible = true;
}

void dev::BaseWindow::SetVisibility(const bool _visible)
{
	m_visible = _visible;
}

void dev::BaseWindow::SetWindowDefaultPosSize()
{
	auto windowPos = ImGui::GetWindowPos();
	ImGui::SetNextWindowPos(ImVec2(windowPos.x, windowPos.y), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(m_defaultW, m_defaultH), ImGuiCond_FirstUseEver);
}
