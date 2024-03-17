#include "DisplayWindow.h"
#include <gl/GL.h>

dev::DisplayWindow::DisplayWindow(Hardware& _hardware,
        const float* const _fontSizeP, const float* const _dpiScaleP)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
    m_hardware(_hardware)
{
    CreateTexture(true);
}

void dev::DisplayWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Display", &open, ImGuiWindowFlags_NoCollapse);

	DrawDisplay();

	ImGui::End();
}

void dev::DisplayWindow::DrawDisplay()
{

    if (m_frameTextureId)
    {
        CreateTexture(true);
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