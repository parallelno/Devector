#include "ui/mem_display_window.h"
#include "utils/imgui_utils.h"


dev::MemDisplayWindow::MemDisplayWindow(
	Hardware& _hardware,
	Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool& _visible, const float* const _dpiScaleP, GLUtils& _glUtils,
	const std::string& _vtxShaderS,
	const std::string& _fragShaderS)
	:
	BaseWindow("Memory Display", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visible, _dpiScaleP,
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_HorizontalScrollbar),
	m_hardware(_hardware), m_debugger(_debugger), m_glUtils(_glUtils)
{
	m_isGLInited = Init(_vtxShaderS, _fragShaderS);


	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::HW_RUNNING | dev::Signals::BREAK,
			std::bind(&dev::MemDisplayWindow::CallbackUpdateData,
				this, std::placeholders::_1, std::placeholders::_2),
			m_visible, 10ms));
}

bool dev::MemDisplayWindow::Init(
	const std::string& _vtxShaderS,
	const std::string& _fragShaderS)
{
	// setting up the mem view rendering
	auto memViewShaderId =
		m_glUtils.InitShader(_vtxShaderS.c_str(), _fragShaderS.c_str());

	if (memViewShaderId == INVALID_ID) return false;

	m_memViewShaderId = memViewShaderId;

	for (int i = 0; i < RAM_TEXTURES; i++){
		// ram
		auto memViewTexId = m_glUtils.InitTexture(
			RAM_TEXTURE_W, RAM_TEXTURE_H, GLUtils::Texture::Format::R8);

		if (memViewTexId == INVALID_ID) return false;

		m_memViewTexIds[i] = memViewTexId;
		// highlight reads + writes
		auto lastRWTexId = m_glUtils.InitTexture(
			RAM_TEXTURE_W, RAM_TEXTURE_H, GLUtils::Texture::Format::RGBA);

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

	m_paramId_globalColorBg = m_glUtils.GetMaterialParamId(
		m_memViewMatIds[0], "globalColorBg");

	m_paramId_globalColorFg = m_glUtils.GetMaterialParamId(
		m_memViewMatIds[0], "globalColorFg");

	m_paramId_highlightRead = m_glUtils.GetMaterialParamId(
		m_memViewMatIds[0], "highlightRead");

	m_paramId_highlightWrite = m_glUtils.GetMaterialParamId(
		m_memViewMatIds[0], "highlightWrite");

	m_paramId_highlightIdxMax = m_glUtils.GetMaterialParamId(
		m_memViewMatIds[0], "highlightIdxMax");

	return true;
}

void dev::MemDisplayWindow::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	DrawSelector();
	DrawMemoryTabs();
}


void dev::MemDisplayWindow::DrawSelector()
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
		"Left Ctrl + Mouse Wheel - zoom.\n"
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
}

static const char* separatorsS[] = {
	"Main Ram",
	"RAM Disk 1 Page 0",
	"RAM Disk 1 Page 1",
	"RAM Disk 1 Page 2",
	"RAM Disk 1 Page 3",
	"RAM Disk 2 Page 0",
	"RAM Disk 2 Page 1",
	"RAM Disk 2 Page 2",
	"RAM Disk 2 Page 3",
	"RAM Disk 3 Page 0",
	"RAM Disk 3 Page 1",
	"RAM Disk 3 Page 2",
	"RAM Disk 3 Page 3",
	"RAM Disk 4 Page 0",
	"RAM Disk 4 Page 1",
	"RAM Disk 4 Page 2",
	"RAM Disk 4 Page 3",
	"RAM Disk 5 Page 0",
	"RAM Disk 5 Page 1",
	"RAM Disk 5 Page 2",
	"RAM Disk 5 Page 3",
	"RAM Disk 6 Page 0",
	"RAM Disk 6 Page 1",
	"RAM Disk 6 Page 2",
	"RAM Disk 6 Page 3",
	"RAM Disk 7 Page 0",
	"RAM Disk 7 Page 1",
	"RAM Disk 7 Page 2",
	"RAM Disk 7 Page 3",
	"RAM Disk 8 Page 0",
	"RAM Disk 8 Page 1",
	"RAM Disk 8 Page 2",
	"RAM Disk 8 Page 3"
};


static const char* tab_names[] = {
	"Main Ram",
	"RAM Disk 1",
	"RAM Disk 2",
	"RAM Disk 3",
	"RAM Disk 4",
	"RAM Disk 5",
	"RAM Disk 6",
	"RAM Disk 7",
	"RAM Disk 8"
};

void dev::MemDisplayWindow::DrawMemoryTabs()
{
	if (!m_isGLInited) return;

	m_selectedTab = 0;
	int hovered_bank_idx = -1;
	static ImVec2 img_pixel_pos;

	if (ImGui::BeginTabBar("MemoryPanels"))
	{
		for (int tab_idx = 0; tab_idx < MEM_TABS; tab_idx++)
		{
			if (ImGui::BeginTabItem(tab_names[tab_idx]))
			{
				m_selectedTab = tab_idx;

				ImVec2 mousePos = ImGui::GetMousePos();

				ImVec2 remainingSize = ImGui::GetContentRegionAvail();
				ImGui::BeginChild("ScrollingFrame",
					{ remainingSize.x, remainingSize.y },
					true,
					ImGuiWindowFlags_HorizontalScrollbar);

				ImVec2 imageSize(
					FRAME_BUFFER_W * m_scale,
					FRAME_BUFFER_H * m_scale);

				int bank_idx_start = tab_idx == 0 ?
					0 :	// the main ram, one texture
					1 + (tab_idx - 1) * RAMDISK_PAGES_MAX;

				int banks_len = tab_idx == 0 ? 1 : RAMDISK_PAGES_MAX;


				for (int bank_idx = bank_idx_start;
					bank_idx < bank_idx_start + banks_len;
					bank_idx++)
				{
					ImGui::SeparatorText(separatorsS[bank_idx]);
					ImVec2 imagePos = ImGui::GetCursorScreenPos();

					auto framebufferTex = m_glUtils.GetFramebufferTexture(
						m_memViewMatIds[bank_idx]);

					ImGui::Image(framebufferTex, imageSize);
					hovered_bank_idx = ImGui::IsItemHovered() ?
						bank_idx :
						hovered_bank_idx;

					if (hovered_bank_idx == bank_idx)
					{
						img_pixel_pos = ImVec2(
							mousePos.x - imagePos.x,
							mousePos.y - imagePos.y);
					}


					// if clicked, show the addr in the Hex Window
					if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
					{
						GlobalAddr globalAddr = PixelPosToAddr(img_pixel_pos, m_scale) +
							hovered_bank_idx * Memory::MEM_64K;

						m_scheduler.AddSignal(
							{dev::Signals::HEX_HIGHLIGHT_ON,
							dev::Scheduler::GlobalAddrLen(globalAddr, 1)});
					}
				}

				ImGui::EndChild();

				ImGui::EndTabItem();
			}
		}
	}
	ImGui::EndTabBar();


	if (hovered_bank_idx >= 0){
		ScaleView();
		DrawTooltip(hovered_bank_idx, img_pixel_pos);
	}
}


void dev::MemDisplayWindow::DrawTooltip(
	const int _img_hovered_idx, const ImVec2& _img_pixel_pos)
{
	// Addr Tooltip
	if (!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
	{
		ImGui::BeginTooltip();
		GlobalAddr globalAddr = PixelPosToAddr(_img_pixel_pos, m_scale) +
										_img_hovered_idx * Memory::MEM_64K;

		uint8_t val = m_hardware.Request(Hardware::Req::GET_BYTE_GLOBAL,
			{ { "globalAddr", globalAddr } })->at("data");

		ImGui::Text("0x%06X (0x%02X), %s",
			globalAddr, val, separatorsS[_img_hovered_idx]);

		ImGui::EndTooltip();
	}

	// tooltip zoom
	DrawTooltipTimer();
}


auto dev::MemDisplayWindow::PixelPosToAddr(ImVec2 _pos, float _scale)
-> Addr
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


// check the keys, scale the view
void dev::MemDisplayWindow::ScaleView()
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



void dev::MemDisplayWindow::CallbackUpdateData(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	// update
	if (m_isGLInited)
	{
		auto memP = m_hardware.GetRam()->data();

		m_debugger.UpdateLastRW();
		auto memLastRWP = m_debugger.GetLastRW()->data();

		// update params and vram texture
		int bank_idx_start = m_selectedTab == 0 ?
			0 :	// the main ram, one texture
			1 + (m_selectedTab - 1) * RAMDISK_PAGES_MAX;
		int banks_len = m_selectedTab == 0 ? 1 : RAMDISK_PAGES_MAX;

		for (int i = bank_idx_start; i < bank_idx_start + banks_len; i++)
		{
			m_glUtils.UpdateMaterialParam(
				m_memViewMatIds[i], m_paramId_globalColorBg, m_globalColorBg);

			m_glUtils.UpdateMaterialParam(
				m_memViewMatIds[i], m_paramId_globalColorFg, m_globalColorFg);

			m_glUtils.UpdateMaterialParam(
				m_memViewMatIds[i], m_paramId_highlightRead, m_highlightRead);

			m_glUtils.UpdateMaterialParam(
				m_memViewMatIds[i], m_paramId_highlightWrite, m_highlightWrite);

			m_glUtils.UpdateMaterialParam(
				m_memViewMatIds[i], m_paramId_highlightIdxMax, m_highlightIdxMax);

			m_glUtils.UpdateTexture(
				m_memViewTexIds[i], memP + i * Memory::MEM_64K);

			m_glUtils.UpdateTexture(
				m_lastRWTexIds[i],
				(const uint8_t*)(memLastRWP) + i * Memory::MEM_64K * 4);

			m_glUtils.Draw(m_memViewMatIds[i]);
		}
	}
}