#include "ui/mem_display_window.h"
#include "utils/imgui_utils.h"

// #include "imgui_impl_opengl3.h"
// #include "imgui_impl_opengl3_loader.h"

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

	uniform sampler2D texture0; // global ram values
	uniform sampler2D texture1; // .xy - highlight reads, .zw - highlight writes
	uniform vec4 globalColorBg;
	uniform vec4 globalColorFg;
	uniform vec4 highlightRead;
	uniform vec4 highlightWrite;
	uniform vec4 highlightIdxMax;

	layout (location = 0) out vec4 out0;

	#define BYTE_COLOR_MULL 0.6
	#define BACK_COLOR_MULL 0.7

	int GetBit(float _val, int _bitIdx) {
		return (int(_val * 255.0) >> _bitIdx) & 1;
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

		// highlight every second column
		int isByteOdd = (int(uv0.x * 512.0)>>2) & 1;
		vec3 byteColor = mix(globalColorFg.xyz * BYTE_COLOR_MULL, globalColorFg.xyz, float(isByteOdd));

		vec3 color = mix(bgColor, byteColor, float(isBitOn));

		// highlight
		vec4 rw = texture(texture1, uv);
		float reads = (rw[1] * 256.0f + rw[0]) * 256.0f / highlightIdxMax.x;
		float writes = (rw[3] * 255.0f + rw[2] ) * 256.0f / highlightIdxMax.x;
		vec3 readsColor = reads * highlightRead.rgb * highlightRead.a;
		vec3 writesColor = writes * highlightWrite.rgb * highlightWrite.a;
		vec3 rwColor = readsColor + writesColor;
		color = mix(color * 0.8f, color * 0.5f + rwColor * 0.5f + color * rwColor * 2.0f, rwColor);




		out0 = vec4(color, globalColorBg.a);
	}
)";

dev::MemDisplayWindow::MemDisplayWindow(Hardware& _hardware, Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool& _visible, const float* const _dpiScaleP, GLUtils& _glUtils,
	ReqUI& _reqUI)
	:
	BaseWindow("Memory Display", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visible, _dpiScaleP,
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_HorizontalScrollbar),
	m_hardware(_hardware), m_debugger(_debugger), m_glUtils(_glUtils),
	m_reqUI(_reqUI)
{
	m_isGLInited = Init();
}

bool dev::MemDisplayWindow::Init()
{
	// setting up the mem view rendering
	auto memViewShaderId = m_glUtils.InitShader(memViewShaderVtx, memViewShaderFrag);
	if (memViewShaderId == INVALID_ID) return false;
	m_memViewShaderId = memViewShaderId;

	for (int i = 0; i < RAM_TEXTURES; i++){
		// ram
		auto memViewTexId = m_glUtils.InitTexture(RAM_TEXTURE_W, RAM_TEXTURE_H, GLUtils::Texture::Format::R8);
		if (memViewTexId == INVALID_ID) return false;
		m_memViewTexIds[i] = memViewTexId;
		// highlight reads + writes
		auto lastRWTexId = m_glUtils.InitTexture(RAM_TEXTURE_W, RAM_TEXTURE_H, GLUtils::Texture::Format::RGBA);
		if (lastRWTexId == INVALID_ID) return false;
		m_lastRWTexIds[i] = lastRWTexId;
	}

	GLUtils::ShaderParams memViewShaderParams = {
		{ "globalColorBg", m_globalColorBg },
		{ "globalColorFg", m_globalColorFg },
		{ "highlightRead", m_highlightRead },
		{ "highlightWrite", m_highlightWrite },
		{ "highlightIdxMax", m_highlightIdxMax },
	};

	for (int i = 0; i < RAM_TEXTURES; i++)
	{
		auto matId = m_glUtils.InitMaterial(m_memViewShaderId,
			{m_memViewTexIds[i], m_lastRWTexIds[i]}, memViewShaderParams,
			FRAME_BUFFER_W, FRAME_BUFFER_H);

		if (matId == INVALID_ID) return false;

		m_memViewMatIds[i] = matId;
	}

	m_paramId_globalColorBg = m_glUtils.GetMaterialParamId(m_memViewMatIds[0], "globalColorBg");
	m_paramId_globalColorFg = m_glUtils.GetMaterialParamId(m_memViewMatIds[0], "globalColorFg");
	m_paramId_highlightRead = m_glUtils.GetMaterialParamId(m_memViewMatIds[0], "highlightRead");
	m_paramId_highlightWrite = m_glUtils.GetMaterialParamId(m_memViewMatIds[0], "highlightWrite");
	m_paramId_highlightIdxMax = m_glUtils.GetMaterialParamId(m_memViewMatIds[0], "highlightIdxMax");

	return true;
}

void dev::MemDisplayWindow::Draw(const dev::Scheduler::Signals _signals)
{
	bool isRunning = dev::Scheduler::Signals::HW_RUNNING & _signals;

	UpdateData(isRunning);
	DrawDisplay();
}

static const char* separatorsS[] = {
	"The Main Ram",
	"The RAM Disk 1 Page 0",
	"The RAM Disk 1 Page 1",
	"The RAM Disk 1 Page 2",
	"The RAM Disk 1 Page 3",
	"The RAM Disk 2 Page 0",
	"The RAM Disk 2 Page 1",
	"The RAM Disk 2 Page 2",
	"The RAM Disk 2 Page 3",
	"The RAM Disk 3 Page 0",
	"The RAM Disk 3 Page 1",
	"The RAM Disk 3 Page 2",
	"The RAM Disk 3 Page 3",
	"The RAM Disk 4 Page 0",
	"The RAM Disk 4 Page 1",
	"The RAM Disk 4 Page 2",
	"The RAM Disk 4 Page 3",
	"The RAM Disk 5 Page 0",
	"The RAM Disk 5 Page 1",
	"The RAM Disk 5 Page 2",
	"The RAM Disk 5 Page 3",
	"The RAM Disk 6 Page 0",
	"The RAM Disk 6 Page 1",
	"The RAM Disk 6 Page 2",
	"The RAM Disk 6 Page 3",
	"The RAM Disk 7 Page 0",
	"The RAM Disk 7 Page 1",
	"The RAM Disk 7 Page 2",
	"The RAM Disk 7 Page 3",
	"The RAM Disk 8 Page 0",
	"The RAM Disk 8 Page 1",
	"The RAM Disk 8 Page 2",
	"The RAM Disk 8 Page 3"
};

dev::Addr PixelPosToAddr(ImVec2 _pos, float _scale)
{
	// (0,0) is the left-top corner
	int imgX = int(_pos.x / _scale);
	int imgY = int(_pos.y / _scale);

	// img size (1024, 512)
	int addrOffsetH = imgY / 256; // addrOffsetH = 0 means addr is <= 32K, addrOffsetH = 1 means addr is > 32K,
	int eigthKBankIdx = imgX / 256 + 4 * addrOffsetH;

	int eigthKBankPosX = imgX % 256;
	int eigthKBankPosY = imgY % 256;

	dev::Addr addr = ((eigthKBankPosX>>3) * 256 + (255 - eigthKBankPosY)) + eigthKBankIdx * 1024 * 8;

	return addr;
}

void dev::MemDisplayWindow::DrawDisplay()
{
	static int highlightMode = 0; // 0 - RW, 1 - R, 2 - W
	ImGui::Text("Highlight: "); ImGui::SameLine();
	ImGui::RadioButton("RW", &highlightMode, 0); ImGui::SameLine();
	ImGui::RadioButton("R", &highlightMode, 1); ImGui::SameLine();
	ImGui::RadioButton("W", &highlightMode, 2); ImGui::SameLine();
	dev::DrawHelpMarker(
		"Blue highlights represent reads.\n"
		"Red highlights represent writes.\n"
		"The brighter the color, the more recent the change.\n"
		"Left Alt + Mouse Wheel - zoom.\n"
		"The Left Mouse Button to navigate the address in the Hex Window."
		);

	switch (highlightMode)
	{
	case 0:
		m_highlightRead.w = 1.0f;
		m_highlightWrite.w = 1.0f;
		break;
	case 1:
		m_highlightRead.w = 1.0f;
		m_highlightWrite.w = 0.0f;
		break;
	case 2:
		m_highlightRead.w = 0.0f;
		m_highlightWrite.w = 1.0f;
		break;
	default:
		break;
	}

	ImVec2 mousePos = ImGui::GetMousePos();
	static ImVec2 imgPixelPos;
	static int imageHoveredId = 0;

	ImVec2 remainingSize = ImGui::GetContentRegionAvail();
	ImGui::BeginChild("ScrollingFrame", { remainingSize.x, remainingSize.y }, true, ImGuiWindowFlags_HorizontalScrollbar);

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

			if (isInsideImg)
			{
				imgPixelPos = ImVec2(mousePos.x - imagePos.x, mousePos.y - imagePos.y);
				imageHoveredId = i;
			}

			auto framebufferTex = m_glUtils.GetFramebufferTexture(m_memViewMatIds[i]);
			ImGui::Image(framebufferTex, imageSize);

			// if clicked, show the addr in the Hex Window
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				Addr addr = PixelPosToAddr(imgPixelPos, m_scale);
				m_reqUI.type = ReqUI::Type::HEX_HIGHLIGHT_ON;
				m_reqUI.globalAddr = addr + imageHoveredId * Memory::MEM_64K;
				m_reqUI.len = 1;
			}
		}
	}
	ScaleView();
	ImGui::EndChild();

	// addr pop-up
	if (ImGui::IsItemHovered())
	{
		if (imageHoveredId >= 0 &&
			!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
		{
			ImGui::BeginTooltip();
			GlobalAddr globalAddr = PixelPosToAddr(imgPixelPos, m_scale) + imageHoveredId * Memory::MEM_64K;
			uint8_t val = m_hardware.Request(Hardware::Req::GET_BYTE_GLOBAL, { { "globalAddr", globalAddr } })->at("data");
			ImGui::Text("0x%06X (0x%02X), %s", globalAddr, val, separatorsS[imageHoveredId]);
			ImGui::EndTooltip();
		}

		// tooltip zoom
		DrawTooltipTimer();
	}
}

void dev::MemDisplayWindow::UpdateData(const bool _isRunning)
{
	// check if the hardware updated its state
	uint64_t cc = m_hardware.Request(Hardware::Req::GET_CC)->at("cc");
	auto ccDiff = cc - m_ccLast;
	//if (ccDiff == 0) return;
	m_ccLast = cc;

	// update
	if (m_isGLInited)
	{
		auto memP = m_hardware.GetRam()->data();

		if (ccDiff != 0) m_debugger.UpdateLastRW();
		auto memLastRWP = m_debugger.GetLastRW()->data();

		// update params and vram texture
		for (int i = 0; i < RAM_TEXTURES; i++)
		{
			m_glUtils.UpdateMaterialParam(m_memViewMatIds[i], m_paramId_globalColorBg, m_globalColorBg);
			m_glUtils.UpdateMaterialParam(m_memViewMatIds[i], m_paramId_globalColorFg, m_globalColorFg);
			m_glUtils.UpdateMaterialParam(m_memViewMatIds[i], m_paramId_highlightRead, m_highlightRead);
			m_glUtils.UpdateMaterialParam(m_memViewMatIds[i], m_paramId_highlightWrite, m_highlightWrite);
			m_glUtils.UpdateMaterialParam(m_memViewMatIds[i], m_paramId_highlightIdxMax, m_highlightIdxMax);

			m_glUtils.UpdateTexture(m_memViewTexIds[i], memP + i * Memory::MEM_64K);
			m_glUtils.UpdateTexture(m_lastRWTexIds[i], (const uint8_t*)(memLastRWP) + i * Memory::MEM_64K * 4);
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

			if (ImGui::GetIO().MouseWheel > 0.0f){
				m_scale = dev::Min( m_scale + scaleAdjusted, SCALE_MAX);
			}
			else if (ImGui::GetIO().MouseWheel < 0.0f) {
				m_scale = dev::Max(m_scale - scaleAdjusted, SCALE_MIN);
			}
			if (ImGui::GetIO().MouseWheel != 0.0f) {
				DrawTooltipTimer(std::format("Zoom: {}", m_scale).c_str());
			}
		}
	}
}