#include "RamViewWindow.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_opengl3_loader.h"


dev::RamViewWindow::RamViewWindow(Hardware& _hardware,
        const float* const _fontSizeP, const float* const _dpiScaleP)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
    m_hardware(_hardware), m_glUtils(_hardware)
{}

void dev::RamViewWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Ram View", &open, ImGuiWindowFlags_NoCollapse);

    bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");
    UpdateData(isRunning);

	DrawDisplay();

    ScaleView();
	ImGui::End();
}

void dev::RamViewWindow::DrawDisplay()
{

    if (m_glUtils.IsShaderDataReady())
    {   
        ImVec2 imageSize(GLUtils::FRAME_BUFFER_W * m_scale, GLUtils::FRAME_BUFFER_H * m_scale);

        ImGui::SeparatorText("The Main Ram");
        ImGui::Image((void*)(intptr_t)m_glUtils.GetShaderData()->framebufferTextures[0], imageSize);
        
        ImGui::SeparatorText("The Ram-Disk Page 0");
        ImGui::Image((void*)(intptr_t)m_glUtils.GetShaderData()->framebufferTextures[1], imageSize);
        
        ImGui::SeparatorText("The Ram-Disk Page 1");
        ImGui::Image((void*)(intptr_t)m_glUtils.GetShaderData()->framebufferTextures[2], imageSize);
        
        ImGui::SeparatorText("The Ram-Disk Page 2");
        ImGui::Image((void*)(intptr_t)m_glUtils.GetShaderData()->framebufferTextures[3], imageSize);
        
        ImGui::SeparatorText("The Ram-Disk Page 3");
        ImGui::Image((void*)(intptr_t)m_glUtils.GetShaderData()->framebufferTextures[4], imageSize);
    }
}

void dev::RamViewWindow::UpdateData(const bool _isRunning)
{
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

// check the keys, scale the view
void dev::RamViewWindow::ScaleView()
{
    if (ImGui::IsWindowHovered())
    {
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            float scaleAdjusted = m_scale < 1.0f ? SCALE_INC : m_scale * SCALE_INC;

            if (ImGui::GetIO().MouseWheel > 0.0f)
            {
                m_scale = m_scale > SCALE_MAX ? SCALE_MAX : m_scale + scaleAdjusted;
            }
            else if (ImGui::GetIO().MouseWheel < 0.0f)
            {
                m_scale = m_scale < SCALE_MIN ? SCALE_MIN : m_scale - scaleAdjusted;
            }
        }
    }
}