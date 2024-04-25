#include "MemDisplayWindow.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_opengl3_loader.h"

// Vertex shader source code
const char* rvShaderVtx = R"(
	#version 330 core
	precision highp float;
	
	layout (location = 0) in vec3 vtxPos;
	layout (location = 1) in vec2 vtxUV;
	
	out vec2 uv0;

	void main()
	{
		uv0 = vtxUV;
		gl_Position = vec4(vtxPos.xyz, 1.0f);
	}
)";

// Fragment shader source code
const char* rvShaderFrag = R"(
	#version 330 core
	precision highp float;
	precision highp int;

	in vec2 uv0;

	uniform sampler2D texture0;
	//uniform ivec2 iresolution;
	uniform vec4 globalColorBg;
	uniform vec4 globalColorFg;

	layout (location = 0) out vec4 out0;

	#define BYTE_COLOR_MULL 0.6
	#define BACK_COLOR_MULL 0.7

	int GetBit(float _color, int _bitIdx) {
		return (int(_color * 255.0) >> _bitIdx) & 1;
	}

	void main()
	{
		float isAddrBelow32K = 1.0 - step(0.5, uv0.y);
		vec2 uv = vec2( uv0.y * 2.0, uv0.x / 2.0 + isAddrBelow32K * 0.5);
		float byte = texture(texture0, uv).r;

		float isOdd8K = step(0.5, fract(uv0.x / 0.5));
		isOdd8K = mix(isOdd8K, 1.0 - isOdd8K, isAddrBelow32K);
		vec3 bgColor = mix(globalColorBg.xyz, globalColorBg.xyz * BACK_COLOR_MULL, isOdd8K);

		int bitIdx = 7 - int(uv0.x * 1024.0) & 7;
		int isBitOn = GetBit(byte, bitIdx);

		int isByteOdd = (int(uv0.x * 512.0)>>2) & 1;
		vec3 byteColor = mix(globalColorFg.xyz * BYTE_COLOR_MULL, globalColorFg.xyz, float(isByteOdd));
		vec3 color = mix(bgColor, byteColor, float(isBitOn));

		out0 = vec4(color, globalColorBg.a);
		//out0 = vec4(byte,byte,byte, globalColorBg.a);
	}
)";

dev::MemDisplayWindow::MemDisplayWindow(Hardware& _hardware, Debugger& _debugger,
		const float* const _fontSizeP, const float* const _dpiScaleP, GLUtils& _glUtils)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_hardware(_hardware), m_debugger(_debugger), m_glUtils(_glUtils)
{
	GLUtils::ShaderParams shaderParams = {
		{ "globalColorBg", &m_globalColorBg },
		{ "globalColorFg", &m_globalColorFg } };
	m_renderDataIdx = m_glUtils.InitRenderData(rvShaderVtx, rvShaderFrag, FRAME_BUFFER_W, FRAME_BUFFER_H, shaderParams, RAM_TEXTURES);
}

void dev::MemDisplayWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Memory Display", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar);

	bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");
	UpdateData(isRunning);

	DrawDisplay();

	ImGui::End();
}

static const char* separatorsS[] = {
	"The Main Ram",
	"The Ram-Disk Page 0",
	"The Ram-Disk Page 1",
	"The Ram-Disk Page 2",
	"The Ram-Disk Page 3"
};

dev::Addr PixelPosToAddr(ImVec2 _pos, float _scale) 
{
	int imgX = int(_pos.x / _scale);
	int imgY = int(_pos.y / _scale);

	int addrOffsetH = imgY / 256; // if the cursor hovers the bottom part of the img, the addr is >= 32K
	int eigthKBankIdx = imgX / 256 + 4 * addrOffsetH;

	int eigthKBankPosX = imgX % 256;
	int eigthKBankPosY = imgY % 256;

	int addr = ((eigthKBankPosX>>3) * 256 + (255 - eigthKBankPosY)) + eigthKBankIdx * 1024 * 8;

	return addr;
}

void dev::MemDisplayWindow::DrawDisplay()
{
	ImVec2 mousePos = ImGui::GetMousePos();
	static ImVec2 imgPixelPos;
	static int imageHoveredId = 0;

	std::string labelText = "Hovered Addr: ";
	if (imageHoveredId >= 0) 
	{
		Addr addr = PixelPosToAddr(imgPixelPos, m_scale);
		labelText += std::format("0x{:04X}, {}", addr, separatorsS[imageHoveredId]);
	}
	ImGui::Text(labelText.c_str());

	ImVec2 remainingSize = ImGui::GetContentRegionAvail();
	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 windowEndPos = ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y);
	ImVec2 windowBottomRight = ImVec2(windowEndPos.x, windowPos.y + windowSize.y);

	ImGui::BeginChild("ScrollingFrame", ImVec2(remainingSize.x, remainingSize.y), true, ImGuiWindowFlags_HorizontalScrollbar);

	if (m_renderDataIdx >= 0 && m_glUtils.IsShaderDataReady(m_renderDataIdx))
	{
		auto& framebufferTextures = m_glUtils.GetFramebufferTextures(m_renderDataIdx);
		ImVec2 imageSize(FRAME_BUFFER_W * m_scale, FRAME_BUFFER_H * m_scale);
		imageHoveredId = -1;

		for (int i = 0; i < 5; i++)
		{
			ImGui::SeparatorText(separatorsS[i]);
			ImVec2 imagePos = ImGui::GetCursorScreenPos();
			ImVec2 imageEndPos = ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y);

			bool isInsideImg = mousePos.x >= imagePos.x && mousePos.x < imageEndPos.x &&
				mousePos.y >= imagePos.y && mousePos.y < imageEndPos.y;

			bool isInsideWindow = mousePos.x >= windowPos.x && mousePos.x < windowEndPos.x &&
				mousePos.y >= windowPos.y && mousePos.y < windowBottomRight.y;

			if (isInsideWindow && isInsideImg)
			{
				imgPixelPos = ImVec2(mousePos.x - imagePos.x, mousePos.y - imagePos.y);
				imageHoveredId = i;
			}

			ImGui::Image((void*)(intptr_t)framebufferTextures[i], imageSize);
		}
	}
	ScaleView();
	ImGui::EndChild();
}

void dev::MemDisplayWindow::UpdateData(const bool _isRunning)
{
	// check if the hardware updated its state
	auto res = m_hardware.Request(Hardware::Req::GET_REGS);
	const auto& data = *res;

	uint64_t cc = data["cc"];
	auto ccDiff = cc - m_ccLast;
	if (ccDiff == 0) return;
	m_ccLast = cc;

	// update
	if (m_renderDataIdx >= 0)
	{
		auto memP = m_hardware.GetRam()->data();
		m_glUtils.UpdateTextures(m_renderDataIdx, memP, RAM_TEXTURE_W, RAM_TEXTURE_H, 1);
		m_glUtils.Draw(m_renderDataIdx);
	}

	m_debugger.UpdateLastReads();
	m_debugger.UpdateLastWrites();
}

// check the keys, scale the view
void dev::MemDisplayWindow::ScaleView()
{
	if (ImGui::IsWindowHovered())
	{
		if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
		{
			float scaleAdjusted = m_scale < 1.0f ? SCALE_INC : m_scale * SCALE_INC;

			if (ImGui::GetIO().MouseWheel > 0.0f)
			{
				m_scale = dev::Min( m_scale + scaleAdjusted, SCALE_MAX);
			}
			else if (ImGui::GetIO().MouseWheel < 0.0f)
			{
				m_scale = dev::Max(m_scale - scaleAdjusted, SCALE_MIN);
			}
		}
	}
}