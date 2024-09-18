#include "ui/keyboard_window.h"

#include <format>
#include "utils/str_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

dev::KeyboardWindow::KeyboardWindow(Hardware& _hardware,
	const float* const _dpiScaleP, GLUtils& _glUtils,
	ReqUI& _reqUI, const std::string& _pathImgKeyboard)
	:
	BaseWindow("Keyboard", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _dpiScaleP),
	m_hardware(_hardware), m_glUtils(_glUtils),
	m_reqUI(_reqUI), m_pathImgKeyboard(_pathImgKeyboard)
{
	m_isGLInited = Init();
}

bool dev::KeyboardWindow::Init()
{
	auto dataP = stbi_load(m_pathImgKeyboard.c_str(), &m_imgKeyboardW, &m_imgKeyboardH, &m_imgKeyboardCh, 0);
	if (!dataP){
		dev::Log("Keyboard Window: Failed to load keyboard image. Reason: {}\nPath: {}", stbi_failure_reason(), m_pathImgKeyboard);
		return false;
	}
/*
	auto vramShaderRes = m_glUtils.InitShader(vtxShaderS, fragShaderS);
	if (!vramShaderRes) return false;
	m_vramShaderId = *vramShaderRes;
	*/

	auto m_vramTexRes = m_glUtils.InitTexture(m_imgKeyboardW, m_imgKeyboardH, GLUtils::Texture::Format::RGB);
	if (!m_vramTexRes) return false;
	m_vramTexId = *m_vramTexRes;

	m_glUtils.UpdateTexture(m_vramTexId, (uint8_t*)dataP);
	/*
	m_glUtils.Draw(m_vramMatId);

	GLUtils::ShaderParams shaderParams = {
		{ "m_activeArea_pxlSize", &m_activeArea_pxlSize },
		{ "m_scrollV_crtXY_highlightMul", &m_scrollV_crtXY_highlightMul },
		{ "m_bordsLRTB", &m_bordsLRTB }};
	auto vramMatRes = m_glUtils.InitMaterial(m_vramShaderId, Display::FRAME_W, Display::FRAME_H,
			{m_vramTexRes}, shaderParams);
	if (!vramMatRes) return false;
	m_vramMatId = *vramMatRes;
*/
	return true;
}

void dev::KeyboardWindow::Update(bool& _visible)
{
	BaseWindow::Update();

	if (_visible && ImGui::Begin(m_name.c_str(), &_visible, ImGuiWindowFlags_NoCollapse))
	{
		bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");

		Draw(isRunning);

		ImGui::End();
	}
}

void dev::KeyboardWindow::Draw(const bool _isRunning)
{
	if (m_isGLInited)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		auto wMax = ImGui::GetWindowWidth() - style.FramePadding.x * 4;
		auto hMax = ImGui::GetWindowHeight() - style.FramePadding.y * 14;
		
		ImVec2 displaySize;
		displaySize.x = wMax;
		displaySize.y = displaySize.x * KEYBOARD_IMG_ASPECT;
		if (displaySize.y > hMax) 
		{
			displaySize.y = hMax;
			displaySize.x = displaySize.y / KEYBOARD_IMG_ASPECT;
		}

		//auto framebufferTex = m_glUtils.GetFramebufferTexture(m_vramMatId);
		ImGui::Image((void*)(intptr_t)m_vramTexId, displaySize);
		m_displayIsHovered = ImGui::IsItemHovered();
		
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			//ImGui::OpenPopup(m_contextMenuName);
		}
	}
}


void dev::KeyboardWindow::DrawContextMenu()
{
	/*
	if (ImGui::BeginPopup(m_contextMenuName))
	{
		if (ImGui::BeginMenu("Display Settings"))
		{
			ImGui::Combo("Border Type", (int*)(&m_borderType), m_borderTypeS);
			ImGui::Combo("Display Size", (int*)(&m_displaySize), m_displaySizeS);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Emulation Settings"))
		{
			if (ImGui::Combo("Cpu Speed", (int*)(&m_execSpeed), m_execSpeedsS)) 
			{
				m_hardware.Request(Hardware::Req::SET_CPU_SPEED, { {"speed", int(m_execSpeed)} });
			};
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			ImGui::Text(
				"Left Ctrl + S - set the display size\n"
				"Left Ctrl + B - set the border\n"
			);
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}
	*/
}