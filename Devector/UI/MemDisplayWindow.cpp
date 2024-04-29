#include "MemDisplayWindow.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_opengl3_loader.h"

const char* highlightShaderVtx = R"(
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

const char* highlightShaderFrag = R"(
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

const char* memViewShaderVtx = R"(
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

const char* memViewShaderFrag = R"(
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
	m_isGLInited = Init();
}

bool dev::MemDisplayWindow::Init()
{
	auto memViewShaderRes = m_glUtils.InitShader(memViewShaderVtx, memViewShaderFrag);
	if (!memViewShaderRes) return false;
	m_memViewShaderId = *memViewShaderRes;

	auto highlightShaderRes = m_glUtils.InitShader(highlightShaderVtx, highlightShaderFrag);
	if (!highlightShaderRes) return false;
	m_highlightShaderId = *highlightShaderRes;

	for (int i = 0; i < RAM_TEXTURES; i++){
		auto res = m_glUtils.InitTexture(RAM_TEXTURE_W, RAM_TEXTURE_H, GLUtils::Texture::Format::R8);
		if (!res) return false;
		m_memViewTexIds[i] = *res;
	}

	auto lastReadsRes = m_glUtils.InitTexture(Debugger::LAST_RW_W, Debugger::LAST_RW_H, GLUtils::Texture::Format::R32);
	if (!lastReadsRes) return false;
	m_lastReadsTexId = *lastReadsRes;

	auto lastWritesRes = m_glUtils.InitTexture(Debugger::LAST_RW_W, Debugger::LAST_RW_H, GLUtils::Texture::Format::R32);
	if (!lastWritesRes) return false;
	m_lastWritesTexId = *lastWritesRes;

	// setting up the mem highlight rendering. the resulted testure will be used in the mem view rendering below
	GLUtils::ShaderParams hightlightShaderParams = {
		{ "highlightRead", &m_highlightRead },
		{ "highlightWrite", &m_highlightWrite } };
	for (int i = 0; i < RAM_TEXTURES; i++){
		auto res = m_glUtils.InitMaterial(m_highlightShaderId, FRAME_BUFFER_W, FRAME_BUFFER_H, 
			{m_lastReadsTexId, m_lastWritesTexId}, hightlightShaderParams);
		if (!res) return false;
		m_highlightMatIds[i] = *res;
	}

	// setting up the mem view rendering
	GLUtils::ShaderParams memViewShaderParams = {
		{ "globalColorBg", &m_globalColorBg },
		{ "globalColorFg", &m_globalColorFg } };
	for (int i = 0; i < RAM_TEXTURES; i++){
		auto res = m_glUtils.InitMaterial(m_memViewShaderId, FRAME_BUFFER_W, FRAME_BUFFER_H, 
			{m_memViewTexIds[i]}, memViewShaderParams);		
		if (!res) return false;
		m_memViewMatIds[i] = *res;
	}
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

	if (m_isGLInited)
	{
		ImVec2 imageSize(FRAME_BUFFER_W * m_scale, FRAME_BUFFER_H * m_scale);
		imageHoveredId = -1;

		for (int i = 0; i < RAM_TEXTURES; i++)
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

			auto framebufferTex = m_glUtils.GetFramebufferTexture(m_memViewMatIds[i]);
			ImGui::Image((void*)(intptr_t)framebufferTex, imageSize);
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
	if (m_isGLInited)
	{
		/*
		// update reads & writes textures
		auto lastReadsAddrsP = m_debugger.GetLastReadsAddrs()->data();
		auto lastWritesAddrsP = m_debugger.GetLastWritesAddrs()->data();
		m_glUtils.UpdateTextures(m_memViewHighlightRenderDataId, (const uint8_t*)(lastReadsAddrsP), GL_UNSIGNED_INT, Debugger::LAST_RW_W, Debugger::LAST_RW_H, 1);
		m_glUtils.Draw(m_memViewHighlightRenderDataId);
		*/
		
		// update vram texture
		auto memP = m_hardware.GetRam()->data();
		for (int i = 0; i < RAM_TEXTURES; i++){
			m_glUtils.UpdateTexture(m_memViewTexIds[i], memP + i * Memory::MEM_64K);
			m_glUtils.Draw(m_memViewMatIds[i]);
		}
		

	}
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