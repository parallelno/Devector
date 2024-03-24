#include "RamViewWindow.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_opengl3_loader.h"


dev::RamViewWindow::RamViewWindow(Hardware& _hardware,
        const float* const _fontSizeP, const float* const _dpiScaleP)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
    m_hardware(_hardware), m_glUtils(_hardware, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H)
{}

void dev::RamViewWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Ram View", &open, ImGuiWindowFlags_NoCollapse);

    bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");
    UpdateData(isRunning);

	DrawDisplay();

	ImGui::End();
}

void dev::RamViewWindow::DrawDisplay()
{

    if (m_glUtils.IsShaderDataReady())
    {       
        ImVec2 imageSize(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H);
        ImGui::Image((void*)(intptr_t)m_glUtils.GetShaderData()->framebufferTexture, imageSize);
    }
}

void dev::RamViewWindow::UpdateData(const bool _isRunning)
{
    if (!_isRunning) return;

    // check if the hardware updated its state
    auto res = m_hardware.Request(Hardware::Req::GET_REGS);
    const auto& data = *res;

    uint64_t cc = data["cc"];
    auto ccDiff = cc - m_ccLast;
    m_ccLastRun = ccDiff == 0 ? m_ccLastRun : ccDiff;
    m_ccLast = cc;
    if (ccDiff == 0) return;

    // update
    m_glUtils.Update();
}