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
        /*
        ImGui::InvisibleButton("canvas", imageSize);

        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        draw_list->PushClipRect(p0, p1);
        draw_list->AddCallback([](const ImDrawList* _drawList, const ImDrawCmd* _callbackData)
            {
                auto shaderDataP = (GLUtils::ShaderData*)(_callbackData->UserCallbackData);
                auto shaderProgram = shaderDataP->shaderProgram;
                //auto textureId = shaderDataP->frameTextureId;

                ImDrawData* draw_data = ImGui::GetDrawData();
                float L = draw_data->DisplayPos.x;
                float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
                float T = draw_data->DisplayPos.y;
                float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

                const float ortho_projection[4][4] =
                {
                   { 2.0f / (R - L),   0.0f,         0.0f,   0.0f },
                   { 0.0f,         2.0f / (T - B),   0.0f,   0.0f },
                   { 0.0f,         0.0f,        -1.0f,   0.0f },
                   { (R + L) / (L - R),  (T + B) / (B - T),  0.0f,   1.0f },
                };

                glUseProgram(shaderProgram); // If I remove this line, it works
                //glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "ProjMtx"), 1, GL_FALSE, &ortho_projection[0][0]);
            }, (void*)m_glUtils.GetShaderData());

        draw_list->AddRectFilled(p0, p1, 0xFFFF00FF);
        draw_list->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
        draw_list->PopClipRect();
        */


        /*
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddCallback([](const ImDrawList* _drawList, const ImDrawCmd* _callbackData)
            {
                auto glUtilsP = (GLUtils*)(_callbackData->UserCallbackData);
                glUtilsP->Update();
                

            }, (void*)&m_glUtils);
        draw_list->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
        */
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