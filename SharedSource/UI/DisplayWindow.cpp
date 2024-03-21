#include "DisplayWindow.h"

#include "imgui.h"
#include "imgui_impl_opengl3_loader.h"


dev::DisplayWindow::DisplayWindow(Hardware& _hardware,
        const float* const _fontSizeP, const float* const _dpiScaleP)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
    m_hardware(_hardware)
{
    CreateTexture(true);
    UpdateData(false);
}

void dev::DisplayWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Display", &open, ImGuiWindowFlags_NoCollapse);

    bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");
    UpdateData(isRunning);
	
    DrawDisplay();

	ImGui::End();
}

void dev::DisplayWindow::DrawDisplay()
{
    if (m_frameTextureId)
    {
        ImGui::Image((void*)(intptr_t)m_frameTextureId, ImVec2(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H));
    }
}

// creates a textre
void dev::DisplayWindow::CreateTexture(const bool _vsync)
{
    auto frameP = m_hardware.GetFrame(_vsync);

    // Create a OpenGL texture identifier
    if (!m_frameTextureId)
    {
        glGenTextures(1, &m_frameTextureId);
    }
    glBindTexture(GL_TEXTURE_2D, m_frameTextureId);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Display::FRAME_W, Display::FRAME_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, frameP->data());
}

void dev::DisplayWindow::UpdateData(const bool _isRunning)
{
    if (!_isRunning) return;

    auto res = m_hardware.Request(Hardware::Req::GET_REGS);
    const auto& data = *res;

    uint64_t cc = data["cc"];
    auto ccDiff = cc - m_ccLast;
    m_ccLastRun = ccDiff == 0 ? m_ccLastRun : ccDiff;
    m_ccLast = cc;
    if (ccDiff == 0) return;

    // update
    CreateTexture(true);
}