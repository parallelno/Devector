#include <format>
#include "HardwareStatsWindow.h"
#include "Utils\StringUtils.h"

dev::HardwareStatsWindow::HardwareStatsWindow(Hardware& _hardware, 
		const float* const _fontSizeP, const float* const _dpiScaleP)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_hardware(_hardware)
{}

void dev::HardwareStatsWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Hardware Stats", &open, ImGuiWindowFlags_NoCollapse);

	DrawStats();

	ImGui::End();
}

void dev::HardwareStatsWindow::DrawRegs()
{
	auto regs = m_hardware.Request(Hardware::Req::GET_REGS);
	Addr regAF = regs["af"];
	Addr regBC = regs["bc"];
	Addr regDE = regs["de"];
	Addr regHL = regs["hl"];
	Addr regSP = regs["sp"];
	Addr regPC = regs["pc"];

	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_NoPadInnerX |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("regs", 2, flags))
	{
		ImGui::TableSetupColumn("regsNames", ImGuiTableColumnFlags_WidthFixed, 30);

		DrawProperty2("AF", std::format("{:04X}", regAF));
		DrawProperty2("BC", std::format("{:04X}", regBC));
		DrawProperty2("DE", std::format("{:04X}", regDE));
		DrawProperty2("HL", std::format("{:04X}", regHL));
		DrawProperty2("SP", std::format("{:04X}", regSP));
		DrawProperty2("PC", std::format("{:04X}", regPC));

		ImGui::EndTable();
	}
}

void dev::HardwareStatsWindow::DrawFlags()
{
	auto regs = m_hardware.Request(Hardware::Req::GET_REGS);
	bool flagC = regs["flagC"];
	bool flagZ = regs["flagZ"];
	bool flagP = regs["flagP"];
	bool flagS = regs["flagS"];
	bool flagAC		= regs["flagAC"];
	bool flagINTE	= regs["flagINTE"];
	bool flagIFF	= regs["flagIFF"];
	bool flagHLTA	= regs["flagHLTA"];

	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_NoPadInnerX |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("flags", 2, flags))
	{		
		ImGui::TableSetupColumn("flagsNames", ImGuiTableColumnFlags_WidthFixed, 45);

		DrawProperty2("C", dev::BoolToStr(flagC));
		DrawProperty2("Z", dev::BoolToStr(flagZ));
		DrawProperty2("P", dev::BoolToStr(flagP));
		DrawProperty2("S", dev::BoolToStr(flagS));
		DrawProperty2("AC", dev::BoolToStr(flagAC));
		DrawSeparator2("");
		DrawProperty2("INTE", dev::BoolToStr(flagINTE));
		DrawProperty2("IFF", dev::BoolToStr(flagIFF));
		DrawProperty2("HLTA", dev::BoolToStr(flagHLTA));

		ImGui::EndTable();
	}
}

void dev::HardwareStatsWindow::DrawStack()
{
	auto regs = m_hardware.Request(Hardware::Req::GET_REGS);
	Addr regSP = regs["sp"];
	Addr dataAddrN6 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { "addr", regSP - 8 })["data"];
	Addr dataAddrN4 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { "addr", regSP - 6 })["data"];
	Addr dataAddrN2 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { "addr", regSP - 4 })["data"];
	Addr dataAddr0  = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { "addr", regSP - 2 })["data"];
	Addr dataAddrP2 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { "addr", regSP     })["data"];
	Addr dataAddrP4 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { "addr", regSP + 2 })["data"];
	Addr dataAddrP6 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { "addr", regSP + 4 })["data"];

	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_NoPadInnerX |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("stack", 2, flags))
	{
		ImGui::TableSetupColumn("stackAddrs", ImGuiTableColumnFlags_WidthFixed, 30);

		// Stack		
		DrawProperty2("-4", std::format("{:04X}", dataAddrN6));
		DrawProperty2("-6", std::format("{:04X}", dataAddrN4));
		DrawProperty2("-2", std::format("{:04X}", dataAddrN2));
		DrawProperty2("SP", std::format("{:04X}", dataAddr0));
		DrawProperty2("+2", std::format("{:04X}", dataAddrP2));
		DrawProperty2("+4", std::format("{:04X}", dataAddrP4));
		DrawProperty2("+6", std::format("{:04X}", dataAddrP6));

		ImGui::EndTable();
	}
}

void dev::HardwareStatsWindow::DrawHardware()
{
	auto regs = m_hardware.Request(Hardware::Req::GET_REGS);
	uint64_t regCC = regs["cc"];
	auto displayData = m_hardware.Request(Hardware::Req::GET_DISPLAY_DATA);
	int rasterPixel = regs["rasterPixel"];
	int rasterLine = regs["rasterLine"];
	auto memoryModes = m_hardware.Request(Hardware::Req::GET_MEMORY_MODES);
	bool mappingModeStack = memoryModes["mappingModeStack"];
	size_t mappingPageStack = memoryModes["mappingPageStack"];
	uint8_t mappingModeRam = memoryModes["mappingModeRam"];
	uint8_t mappingPageRam = memoryModes["mappingPageRam"];

	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_NoPadInnerX |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("hardware", 2, flags))
	{
		ImGui::TableSetupColumn("hardwareName", ImGuiTableColumnFlags_WidthFixed, 100);
		ImGui::TableSetupColumn("hardwareValue", ImGuiTableColumnFlags_WidthStretch);

		// cpu cycles
		auto lastRunCC = regCC - m_ccLast;
		DrawProperty2("CPU Cicles", std::format("{}", regCC));
		DrawProperty2("Last Run", std::format("{}", lastRunCC));
		DrawProperty2("CRT", std::format("X: {:03} Y: {:03}", rasterPixel, rasterLine));

		// mapping
		DrawSeparator2("Mapping:");
		std::string mappingRamModeS = "Off";

		if ((mappingModeRam & (0x20 + 0x40 + 0x80)) > 0)
		{
			std::string mapping_ram_mode_a = (mappingModeRam & 0x20) > 0 ? "AC" : "--";
			std::string mapping_ram_mode_8 = (mappingModeRam & 0x40) > 0 ? "8" : "-";
			std::string mapping_ram_mode_e = (mappingModeRam & 0x80) > 0 ? "E" : "-";
			mappingRamModeS = mapping_ram_mode_8 + mapping_ram_mode_a + mapping_ram_mode_e;
		}

		DrawProperty2("RAM Mode", mappingRamModeS);
		DrawProperty2("RAM Page", std::format("{}", mappingPageRam));
		DrawProperty2("Stack Mode", dev::BoolToStrC(mappingModeStack, true));
		DrawProperty2("Stack Page", std::format("{}", mappingPageStack));

		ImGui::EndTable();
	}
}

void dev::HardwareStatsWindow::DrawStats()
{	
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 0.0f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });

	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Hideable |
		ImGuiTableFlags_ContextMenuInBody;
	if (ImGui::BeginTable("Hardware Stats", 4, flags))
	{
		ImGui::TableSetupColumn("Regs", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 70);
		ImGui::TableSetupColumn("Stack", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("Hardware", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		PushStyleCompact(1.0f, 0.0f);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		DrawRegs();
		ImGui::TableNextColumn();
		DrawFlags();
		ImGui::TableNextColumn();
		DrawStack();
		ImGui::TableNextColumn();
		DrawHardware();
		
		PopStyleCompact();
		
		ImGui::EndTable();
	}
	ImGui::PopStyleVar(2);
}