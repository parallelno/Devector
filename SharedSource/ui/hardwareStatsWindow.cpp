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
	auto res = m_hardware.Request(Hardware::Req::GET_REGS);
	if (res) { 
		const auto& data = *res;
		Addr regAF = data["af"];
		Addr regBC = data["bc"];
		Addr regDE = data["de"];
		Addr regHL = data["hl"];
		Addr regSP = data["sp"];
		Addr regPC = data["pc"];

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
}

void dev::HardwareStatsWindow::DrawFlags()
{
	auto res = m_hardware.Request(Hardware::Req::GET_REGS);
	if (res){
		const auto& data = *res;

		bool flagC = data["flagC"];
		bool flagZ = data["flagZ"];
		bool flagP = data["flagP"];
		bool flagS = data["flagS"];
		bool flagAC = data["flagAC"];
		bool flagINTE = data["flagINTE"];
		bool flagIFF = data["flagIFF"];
		bool flagHLTA = data["flagHLTA"];

		static ImGuiTableFlags tableFlags =
			ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
			ImGuiTableFlags_SizingStretchSame |
			ImGuiTableFlags_NoPadInnerX |
			ImGuiTableFlags_ContextMenuInBody;

		if (ImGui::BeginTable("flags", 2, tableFlags))
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

}

void dev::HardwareStatsWindow::DrawStack()
{
	auto res = m_hardware.Request(Hardware::Req::GET_REGS);
	if (res) {
		const auto& data = *res;
		Addr regSP = data["sp"];

		Addr dataAddrN6 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP - 8 } })->at("data");
		Addr dataAddrN4 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP - 6 } })->at("data");
		Addr dataAddrN2 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP - 4 } })->at("data");
		Addr dataAddr0 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP - 2 } })->at("data");
		Addr dataAddrP2 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP } })->at("data");
		Addr dataAddrP4 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP + 2 } })->at("data");
		Addr dataAddrP6 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP + 4 } })->at("data");

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

}

void dev::HardwareStatsWindow::DrawHardware()
{
	auto res = m_hardware.Request(Hardware::Req::GET_REGS);
	const auto& data = *res;
	uint64_t cc = data["cc"];
	res = m_hardware.Request(Hardware::Req::GET_DISPLAY_DATA);
	const auto& displayData = *res;
	int rasterPixel = data["rasterPixel"];
	int rasterLine = data["rasterLine"];
	res = m_hardware.Request(Hardware::Req::GET_MEMORY_MODES);
	const auto& memoryModes = *res;
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
		m_ccLastRun = cc - m_ccLast == 0 ? m_ccLastRun : cc - m_ccLast;
		m_ccLast = cc;
		DrawProperty2("CPU Cicles", std::format("{}", cc));
		DrawProperty2("Last Run", std::format("{}", m_ccLastRun));
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