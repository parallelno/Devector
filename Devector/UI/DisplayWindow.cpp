#include "DisplayWindow.h"

#include "imgui.h"
#include "imgui_impl_opengl3_loader.h"

// Vertex shader source code
const char* vtxShaderS = R"#(
	#version 330 core
	precision highp float;

	layout (location = 0) in vec3 pos;
	layout (location = 1) in vec2 uv;

	uniform vec4 m_shaderData_bordL_bordB_visBord_bordT;

	out vec2 uv0;

	void main()
	{
		float bordL = 256.0f - m_shaderData_bordL_bordB_visBord_bordT.x;
		float bordB = m_shaderData_bordL_bordB_visBord_bordT.y;
		float visBord = m_shaderData_bordL_bordB_visBord_bordT.z;

		vec2 texPxlSize = vec2(1.0f/768.0f , 1.0f/312.0f);
		
		float visibleArea = 256.0f + visBord * 2.0f;
		vec2 visibleAreaCenter = vec2(bordL + 256.0f, bordB + 128.0f);

		// Inverse y to match the display
		uv0 = vec2(uv.x, 1.0f - uv.y);

		uv0 -= visibleAreaCenter * texPxlSize;
		uv0 *= vec2(visibleArea * 2.0f , visibleArea) * texPxlSize;
		uv0 += vec2(0.5f, 0.5f);

		gl_Position = vec4(pos.xyz, 1.0f);
	}
)#";

// Fragment shader source code
const char* fragShaderS = R"#(
	#version 330 core
	precision highp float;
	precision highp int;

	in vec2 uv0;

	uniform sampler2D texture0;
	uniform vec4 m_shaderData_scrollVert;
	uniform vec4 m_shaderData_bordL_bordB_visBord_bordT;
	uniform vec4 m_crtXY_highlightMul;

	layout (location = 0) out vec4 out0;

	void main()
	{
		vec2 uv = uv0;
		vec2 texPxlSize = vec2(1.0f / 768.0f, 1.0f / 312.0f);

		// vertical scrolling
		float borderLeft = m_shaderData_bordL_bordB_visBord_bordT.x * texPxlSize.x;
		float borderRight = borderLeft + 512.0f * texPxlSize.x;
		float borderTop = m_shaderData_bordL_bordB_visBord_bordT.w *  texPxlSize.y;
		float borderBottom = borderTop + 256.0f * texPxlSize.y;
		if (uv.x >= borderLeft &&
			uv.x < borderRight &&
			uv.y >= borderTop &&
			uv.y < borderBottom)
		{
			uv.y -= m_shaderData_scrollVert.x * texPxlSize.y;
			// wrap V
			uv.y += uv.y < borderTop ? 256.0f * texPxlSize.y : 0.0f;
		}

		vec3 color = texture(texture0, uv).rgb;

		// crt scanline highlight
		if (uv.y > m_crtXY_highlightMul.y * texPxlSize.y ||
			(uv.y >= m_crtXY_highlightMul.y * texPxlSize.y && uv.x > m_crtXY_highlightMul.x * texPxlSize.x))
		{
			color.xyz *= m_crtXY_highlightMul.z;
		}

		out0 = vec4(color, 1.0f);
	}
)#";

dev::DisplayWindow::DisplayWindow(Hardware& _hardware,
		const float* const _fontSizeP, const float* const _dpiScaleP, GLUtils& _glUtils)
	:
	BaseWindow("Display", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_hardware(_hardware), m_isHovered(false), m_glUtils(_glUtils)
{
	m_isGLInited = Init();
}

bool dev::DisplayWindow::Init()
{
	auto vramShaderRes = m_glUtils.InitShader(vtxShaderS, fragShaderS);
	if (!vramShaderRes) return false;
	m_vramShaderId = *vramShaderRes;

	auto m_vramTexRes = m_glUtils.InitTexture(Display::FRAME_W, Display::FRAME_H, GLUtils::Texture::Format::RGBA);
	if (!m_vramTexRes) return false;
	m_vramTexId = *m_vramTexRes;

	GLUtils::ShaderParams shaderParams = {
		{ "m_shaderData_scrollVert", &m_shaderData_scrollVert },
		{ "m_shaderData_bordL_bordB_visBord_bordT", &m_shaderData_bordL_bordB_visBord_bordT },
		{ "m_crtXY_highlightMul", &m_crtXY_highlightMul } };
	auto vramMatRes = m_glUtils.InitMaterial(m_vramShaderId, RENDER_TARGET_W, RENDER_TARGET_H,
			{m_vramTexRes}, shaderParams);
	if (!vramMatRes) return false;
	m_vramMatId = *vramMatRes;

	return true;
}

void dev::DisplayWindow::Update(bool& _visible)
{
	BaseWindow::Update();

	if (_visible && ImGui::Begin(m_name.c_str(), &_visible))
	{
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
}

bool dev::DisplayWindow::IsHovered() const
{
	return m_isHovered;
}

void dev::DisplayWindow::DrawDisplay()
{
	if (m_isGLInited)
	{
		auto framebufferTex = m_glUtils.GetFramebufferTexture(m_vramMatId);
		ImGui::Image((void*)(intptr_t)framebufferTex, ImVec2(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H));
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

	if (!_isRunning && m_isHovered)
	{
		res = m_hardware.Request(Hardware::Req::GET_DISPLAY_DATA);
		const auto& displayData = *res;
		int rasterPixel = displayData["rasterPixel"];
		int rasterLine = displayData["rasterLine"];

		m_crtXY_highlightMul.x = (float)rasterPixel;
		m_crtXY_highlightMul.y = (float)rasterLine;
		m_crtXY_highlightMul.z = SCANLINE_HIGHLIGHT_MUL;
	}
	else {
		m_crtXY_highlightMul.z = 1.0f;
	}

	//if (ccDiff == 0) return;

	// update
	if (m_isGLInited)
	{
		m_shaderData_bordL_bordB_visBord_bordT.x = (float)Display::BORDER_LEFT;
		m_shaderData_scrollVert.x = static_cast<uint8_t>(m_hardware.Request(Hardware::Req::SCROLL_VERT)->at("scrollVert") + 1); // adding +1 offset because the default is 255

		auto frameP = m_hardware.GetFrame(_isRunning);
		m_glUtils.UpdateTexture(m_vramTexId, (uint8_t*)frameP->data());
		m_glUtils.Draw(m_vramMatId);
	}
}