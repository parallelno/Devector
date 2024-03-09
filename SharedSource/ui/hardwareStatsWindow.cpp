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
	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_NoPadInnerX |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("regs", 2, flags))
	{
		ImGui::TableSetupColumn("regsNames", ImGuiTableColumnFlags_WidthFixed, 30);

		DrawProperty2("AF", std::format("{:04X}", m_hardware.m_cpu.GetAF()));
		DrawProperty2("BC", std::format("{:04X}", m_hardware.m_cpu.GetBC()));
		DrawProperty2("DE", std::format("{:04X}", m_hardware.m_cpu.GetDE()));
		DrawProperty2("HL", std::format("{:04X}", m_hardware.m_cpu.GetHL()));
		DrawProperty2("SP", std::format("{:04X}", m_hardware.m_cpu.m_sp));
		DrawProperty2("PC", std::format("{:04X}", m_hardware.m_cpu.m_pc));

		ImGui::EndTable();
	}
}

void dev::HardwareStatsWindow::DrawFlags()
{
	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_NoPadInnerX |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("flags", 2, flags))
	{		
		ImGui::TableSetupColumn("flagsNames", ImGuiTableColumnFlags_WidthFixed, 45);

		DrawProperty2("C", dev::BoolToStr(m_hardware.m_cpu.m_flagC));
		DrawProperty2("Z", dev::BoolToStr(m_hardware.m_cpu.m_flagZ));
		DrawProperty2("P", dev::BoolToStr(m_hardware.m_cpu.m_flagP));
		DrawProperty2("S", dev::BoolToStr(m_hardware.m_cpu.m_flagS));
		DrawProperty2("AC", dev::BoolToStr(m_hardware.m_cpu.m_flagAC));
		DrawSeparator2("");
		DrawProperty2("INTE", dev::BoolToStr(m_hardware.m_cpu.m_INTE));
		DrawProperty2("IFF", dev::BoolToStr(m_hardware.m_cpu.m_IFF));
		DrawProperty2("HLTA", dev::BoolToStr(m_hardware.m_cpu.m_HLTA));

		ImGui::EndTable();
	}
}

void dev::HardwareStatsWindow::DrawStack()
{
	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_NoPadInnerX |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("stack", 2, flags))
	{
		ImGui::TableSetupColumn("stackAddrs", ImGuiTableColumnFlags_WidthFixed, 30);

		// Stack
		auto addrN6 = m_hardware.m_memory.GetWord(m_hardware.m_cpu.m_sp - 8, Memory::AddrSpace::STACK);
		auto addrN4 = m_hardware.m_memory.GetWord(m_hardware.m_cpu.m_sp - 6, Memory::AddrSpace::STACK);
		auto addrN2 = m_hardware.m_memory.GetWord(m_hardware.m_cpu.m_sp - 4, Memory::AddrSpace::STACK);
		auto addr0 = m_hardware.m_memory.GetWord(m_hardware.m_cpu.m_sp - 2, Memory::AddrSpace::STACK);
		auto addrP2 = m_hardware.m_memory.GetWord(m_hardware.m_cpu.m_sp, Memory::AddrSpace::STACK);
		auto addrP4 = m_hardware.m_memory.GetWord(m_hardware.m_cpu.m_sp + 2, Memory::AddrSpace::STACK);
		auto addrP6 = m_hardware.m_memory.GetWord(m_hardware.m_cpu.m_sp + 4, Memory::AddrSpace::STACK);
		
		DrawProperty2("-4", std::format("{:04X}", addrN4));
		DrawProperty2("-6", std::format("{:04X}", addrN6));
		DrawProperty2("-2", std::format("{:04X}", addrN2));
		DrawProperty2("SP", std::format("{:04X}", addr0));
		DrawProperty2("+2", std::format("{:04X}", addrP2));
		DrawProperty2("+4", std::format("{:04X}", addrP4));
		DrawProperty2("+6", std::format("{:04X}", addrP6));

		ImGui::EndTable();
	}
}

void dev::HardwareStatsWindow::DrawHardware()
{
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
		auto lastRunCC = m_hardware.m_cpu.m_cc - m_ccLast;
		DrawProperty2("CPU Cicles", std::format("{}", m_hardware.m_cpu.m_cc));
		DrawProperty2("Last Run", std::format("{}", lastRunCC));
		DrawProperty2("CRT", std::format("X: {:03} Y: {:03}", m_hardware.m_display.m_rasterPixel, m_hardware.m_display.m_rasterLine));

		// mapping
		DrawSeparator2("Mapping:");
		auto mappingModeRam = m_hardware.m_memory.m_mappingModeRam;
		std::string mappingRamModeS = "Off";

		if ((mappingModeRam & (0x20 + 0x40 + 0x80)) > 0)
		{
			std::string mapping_ram_mode_a = (mappingModeRam & 0x20) > 0 ? "AC" : "--";
			std::string mapping_ram_mode_8 = (mappingModeRam & 0x40) > 0 ? "8" : "-";
			std::string mapping_ram_mode_e = (mappingModeRam & 0x80) > 0 ? "E" : "-";
			mappingRamModeS = mapping_ram_mode_8 + mapping_ram_mode_a + mapping_ram_mode_e;
		}

		DrawProperty2("RAM Mode", mappingRamModeS);
		DrawProperty2("RAM Page", std::format("{}", m_hardware.m_memory.m_mappingPageRam));
		DrawProperty2("Stack Mode", dev::BoolToStrC(m_hardware.m_memory.m_mappingModeStack, true));
		DrawProperty2("Stack Page", std::format("{}", m_hardware.m_memory.m_mappingPageStack));

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