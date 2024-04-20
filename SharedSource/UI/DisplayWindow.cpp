#include "DisplayWindow.h"

#include "imgui.h"
#include "imgui_impl_opengl3_loader.h"

// Vertex shader source code
const char* vtxShaderS = R"(
	#version 330 core
	precision highp float;

	layout (location = 0) in vec3 pos;
	layout (location = 1) in vec2 uv;

	uniform vec4 m_shaderData_bordL_bordB_visBord;

	out vec2 uv0;

	void main()
	{
		float bordL = 256.0f - m_shaderData_bordL_bordB_visBord.x;
		float bordB = m_shaderData_bordL_bordB_visBord.y;
		float visBord = m_shaderData_bordL_bordB_visBord.z;

		vec2 texPxlSize = vec2(1.0f/768.0f , 1.0f/312.0f);
		
		float visibleArea = 256.0f + visBord * 2.0f;
		vec2 visibleAreaCenter = vec2(bordL + 256.0f, bordB + 128.0f);

		// iverse y to match the display
		uv0 = vec2(uv.x, 1.0f - uv.y);

		uv0 -= visibleAreaCenter * texPxlSize;
		uv0 *= vec2(visibleArea * 2.0f , visibleArea) * texPxlSize;
		uv0 += vec2(0.5f, 0.5f);

		gl_Position = vec4(pos.xyz, 1.0f);
	}
)";

// Fragment shader source code
const char* fragShaderS = R"(
	#version 330 core
	precision highp float;

	in vec2 uv0;

	uniform sampler2D texture0;
	uniform ivec2 iresolution;
	uniform vec4 m_shaderData_scrollVert;

	layout (location = 0) out vec4 out0;

	void main()
	{
		vec2 uv = uv0;
		float scrollV = 0;

		vec2 texPxlSize = vec2(1.0f/768.0f , 1.0f/312.0f);

		float borderLeft = texPxlSize.x * 128.0f;
		float borderRight = borderLeft + texPxlSize.x * 512.0f;
		float borderTop = texPxlSize.y * 40.0f;
		float borderBottom = borderTop + texPxlSize.y * 256.0f;

		if (uv.x >= borderLeft &&
			uv.x < borderRight &&
			uv.y >= borderTop &&
			uv.y < borderBottom)
		{
			scrollV = 1.0f/255.0f * m_shaderData_scrollVert.x;
		}
		vec2 uvScrolled = vec2(uv.x, uv.y - scrollV);
		vec3 color = texture(texture0, uvScrolled).rgb;
		out0 = vec4(color, 1.0f);
	}
)";

dev::DisplayWindow::DisplayWindow(Hardware& _hardware,
		const float* const _fontSizeP, const float* const _dpiScaleP, GLUtils& _glUtils)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_hardware(_hardware), m_isHovered(false), m_glUtils(_glUtils)
{
	GLUtils::ShaderParams shaderParams = {
		{ "m_shaderData_scrollVert", &m_shaderData_scrollVert },
		{ "m_shaderData_bordL_bordB_visBord", &m_shaderData_bordL_bordB_visBord },
		};

	m_renderDataIdx = m_glUtils.InitRenderData(vtxShaderS, fragShaderS, FRAME_W, FRAME_H, shaderParams, 1, GL_NEAREST1);
}

void dev::DisplayWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin(m_name.c_str(), &open, ImGuiWindowFlags_NoCollapse);

	static int dev_IRQ_COMMIT_PXL = 72;

	//ImGui::SliderInt("OUT_COMMIT_TIME", &(dev::OUT_COMMIT_TIME), 0, 512);
	//ImGui::SliderInt("PALETTE_COMMIT_TIME", &(dev::PALETTE_COMMIT_TIME), 0, 512);
	//ImGui::SliderInt("IRQ_COMMIT_PXL", &dev_IRQ_COMMIT_PXL, 0, 512);
	//ImGui::SliderInt("BORDER_LEFT", &(Display::BORDER_LEFT), 0, 512);

	//TODO: why dev::IRQ_COMMIT_PXL is not changing???
	dev::IRQ_COMMIT_PXL = dev_IRQ_COMMIT_PXL;

	bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");
	UpdateData(isRunning);

	DrawDisplay();

	m_isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_None);
	ImGui::End();
}

bool dev::DisplayWindow::IsHovered() const
{
	return m_isHovered;
}

void dev::DisplayWindow::DrawDisplay()
{
	if (m_renderDataIdx >= 0 && m_glUtils.IsShaderDataReady(m_renderDataIdx))
	{
		m_shaderData_bordL_bordB_visBord.x = (float)Display::BORDER_LEFT;

		float scrollVert = m_hardware.Request(Hardware::Req::SCROLL_VERT)->at("scrollVert");
		m_shaderData_scrollVert.x = scrollVert;

		auto& framebufferTextures = m_glUtils.GetFramebufferTextures(m_renderDataIdx);

		ImGui::Image((void*)(intptr_t)framebufferTextures[0], ImVec2(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H));
	}
}

void dev::DisplayWindow::UpdateData(const bool _isRunning)
{
	auto res = m_hardware.Request(Hardware::Req::GET_REGS);
	const auto& data = *res;

	uint64_t cc = data["cc"];
	auto ccDiff = cc - m_ccLast;
	m_ccLastRun = ccDiff == 0 ? m_ccLastRun : ccDiff;
	m_ccLast = cc;
	if (ccDiff == 0) return;

	// update
	if (m_renderDataIdx >= 0)
	{
		auto frameP = m_hardware.GetFrame(_isRunning);

		m_glUtils.UpdateTextures(m_renderDataIdx, (uint8_t*)frameP->data(), Display::FRAME_W, Display::FRAME_H, sizeof(ColorI), GL_NEAREST1);
		m_glUtils.Draw(m_renderDataIdx);
	}
}