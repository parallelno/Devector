#include "DisplayWindow.h"
#include <gl/GL.h>

dev::DisplayWindow::DisplayWindow(Display& _display, 
        const float* const _fontSizeP, const float* const _dpiScaleP)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_display(_display)
{
    CreateFrameBuffer(m_display.m_data, m_display.FRAME_W, m_display.FRAME_H);
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
        UpdateFrameBuffer();
        ImGui::Image((void*)(intptr_t)m_frameTextureId, ImVec2(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H));
    }
}

void dev::DisplayWindow::CreateFrameBuffer(const ColorI _image[], const int _width, const int _height)
{
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _image);
}

void dev::DisplayWindow::UpdateFrameBuffer()
{
    // TODO: optimization. Consider using glTexSubImage2D, Render-to-texture with FBO, Pixel Buffer Object PBO. 
    CreateFrameBuffer(m_display.m_data, m_display.FRAME_W, m_display.FRAME_H);
}