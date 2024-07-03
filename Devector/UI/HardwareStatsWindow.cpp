#include <format>
#include "HardwareStatsWindow.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/StrUtils.h"

dev::HardwareStatsWindow::HardwareStatsWindow(Hardware& _hardware, 
		const float* const _fontSizeP, const float* const _dpiScaleP, 
		bool& _reset, bool& _ruslat)
	:
	BaseWindow("Hardware Stats", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_hardware(_hardware),
	m_reqHardwareStatsReset(_reset), m_ruslat(_ruslat)
{
	UpdateData(false);
}

void dev::HardwareStatsWindow::Update(bool& _visible)
{
	BaseWindow::Update();

	if (_visible && ImGui::Begin(m_name.c_str(), &_visible, ImGuiWindowFlags_NoCollapse))
	{
		bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");
		UpdateData(isRunning);
		UpdateDataRuntime();
		DrawStats(isRunning);
		ImGui::End();
	}
}

void dev::HardwareStatsWindow::DrawRegs() const
{
	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_NoPadInnerX |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("regs", 2, flags))
	{
		ImGui::TableSetupColumn("regsNames", ImGuiTableColumnFlags_WidthFixed, 30);

		// regs
		DrawProperty2("AF", Uint16ToStrC(m_cpuState.regs.psw.af.word), nullptr, *m_regAFColor);
		DrawProperty2("BC", Uint16ToStrC(m_cpuState.regs.bc.word), nullptr, *m_regBCColor);
		DrawProperty2("DE", Uint16ToStrC(m_cpuState.regs.de.word), nullptr, *m_regDEColor);
		DrawProperty2("HL", Uint16ToStrC(m_cpuState.regs.hl.word), nullptr, *m_regHLColor);
		DrawProperty2("SP", Uint16ToStrC(m_cpuState.regs.sp.word), nullptr, *m_regSPColor);
		DrawProperty2("PC", Uint16ToStrC(m_cpuState.regs.pc.word), nullptr, *m_regPCColor);

		// flags
		ImGui::Dummy({1,8});
		DrawProperty2("C", dev::BoolToStrC(m_cpuState.regs.psw.c, 1), nullptr, *m_flagCColor);
		DrawProperty2("Z", dev::BoolToStrC(m_cpuState.regs.psw.z, 1), nullptr, *m_flagZColor);
		DrawProperty2("P", dev::BoolToStrC(m_cpuState.regs.psw.p, 1), nullptr, *m_flagPColor);
		DrawProperty2("S", dev::BoolToStrC(m_cpuState.regs.psw.s, 1), nullptr, *m_flagSColor);
		DrawProperty2("AC", dev::BoolToStrC(m_cpuState.regs.psw.ac, 1), nullptr, *m_flagACColor);

		ImGui::EndTable();
	}
}

void dev::HardwareStatsWindow::DrawStack() const
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
		DrawProperty2("-12", m_dataAddrN10S.c_str());
		DrawProperty2("-10", m_dataAddrN8S.c_str());
		DrawProperty2("-8", m_dataAddrN6S.c_str());
		DrawProperty2("-6", m_dataAddrN4S.c_str());
		DrawProperty2("-2", m_dataAddrN2S.c_str());
		DrawProperty2("SP", m_dataAddr0S.c_str());
		DrawProperty2("+2", m_dataAddrP2S.c_str());
		DrawProperty2("+4", m_dataAddrP4S.c_str());
		DrawProperty2("+6", m_dataAddrP6S.c_str());
		DrawProperty2("+8", m_dataAddrP8S.c_str());
		DrawProperty2("+10", m_dataAddrP10S.c_str());

		ImGui::EndTable();
	}
}

void dev::HardwareStatsWindow::DrawHardware() const
{
	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_NoPadInnerX |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("hardware", 2, flags))
	{
		ImGui::TableSetupColumn("hwName", ImGuiTableColumnFlags_WidthFixed, 80);

		// cpu cycles
		DrawProperty2("CPU Cicles", m_ccS.c_str());
		DrawProperty2("Last Run", m_ccLastRunS.c_str());
		DrawProperty2("CRT X", m_rasterPixelS.c_str());
		DrawProperty2("CRT Y", m_rasterLineS.c_str());
		DrawProperty2("Rus/Lat", m_ruslatS.c_str());

		// interuption states
		ImGui::Dummy({ 1,8 });
		DrawProperty2("INTE", dev::BoolToStrC(m_cpuState.ints.inte));
		DrawProperty2("IFF", dev::BoolToStrC(m_cpuState.ints.iff));
		DrawProperty2("HLTA", dev::BoolToStrC(m_cpuState.ints.hlta));

		// palette
		dev::DrawSeparator2("Palette");
		float sz = ImGui::GetTextLineHeight();
		for (int i = 0; i < IO::PALETTE_LEN; i++)
		{
			if (i % 8 == 0) ImGui::TableNextRow();
			if (i % 4 == 0)
			{
				ImGui::TableNextColumn();
				ImGui::Dummy({4,4});
				ImGui::SameLine();
			}

			ImVec2 pos = ImGui::GetCursorScreenPos();
			ColorI color = Display::VectorColorToArgb(m_palette.bytes[i]);
			ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + sz, pos.y + sz), color);
			ImGui::Dummy(ImVec2(sz, sz));
			if (ImGui::IsItemHovered()) 
			{
				ImGui::BeginTooltip();
				ImGui::Text("idx: %d, HW Color: 0x%02X, RGB: 0x%06X", i, m_palette.bytes[i], color & 0xFFFFFF);
				ImGui::EndTooltip();
			}
			
			if (i % 4 != 3) ImGui::SameLine();
		}

		// ports
		dev::DrawSeparator2("Ports");
		DrawProperty2("CW", m_cwS.c_str());
		DrawProperty2("Port A", m_portAS.c_str());
		DrawProperty2("Port B", m_portBS.c_str());
		DrawProperty2("Port C", m_portCS.c_str());
		DrawProperty2("CW2", m_cw2S.c_str());
		DrawProperty2("Port A2", m_portA2S.c_str());
		DrawProperty2("Port A2", m_portB2S.c_str());
		DrawProperty2("Port A2", m_portC2S.c_str());

		ImGui::EndTable();
	}
}

void dev::HardwareStatsWindow::DrawPeripheral() const
{
	static ImGuiTableFlags tableFlags =
		ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_NoPadInnerX |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("Peripheral", 2, tableFlags))
	{
		ImGui::TableSetupColumn("pfNames", ImGuiTableColumnFlags_WidthFixed, 110);

		// ram-disk 1 mapping
		DrawSeparator2("Ram-Disk 1:");
		DrawProperty2("RAM Mode", m_mappingRamMode1S.c_str());
		DrawProperty2("RAM Page", m_mappingPageRam1S.c_str());
		DrawProperty2("Stack Mode", m_mappingModeStack1S.c_str());
		DrawProperty2("Stack Page", m_mappingPageStack1S.c_str());
		// ram-disk 2 mapping
		DrawSeparator2("Ram-Disk 2:");
		DrawProperty2("RAM Mode", m_mappingRamMode2S.c_str());
		DrawProperty2("RAM Page", m_mappingPageRam2S.c_str());
		DrawProperty2("Stack Mode", m_mappingModeStack2S.c_str());
		DrawProperty2("Stack Page", m_mappingPageStack2S.c_str());

		// FDC
		DrawSeparator2("FDC:");
		static const char* diskNames[] = { "Drive A", "Drive B", "Drive C", "Drive D" };
		DrawProperty2("Selected", m_fdcDrive.c_str(), m_fdcStats.c_str());

		for (int i = 0; i < Fdc1793::DRIVES_MAX; i++)
		{
			DrawProperty2(diskNames[i], m_fddStats[i].c_str(), m_fddPaths[i].c_str());
		}

		ImGui::EndTable();
	}
}

void dev::HardwareStatsWindow::DrawStats(const bool _isRunning)
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
		ImGui::TableSetupColumn("Stack", ImGuiTableColumnFlags_WidthFixed, 76);
		ImGui::TableSetupColumn("Hardware", ImGuiTableColumnFlags_WidthFixed, 140);
		ImGui::TableSetupColumn("Peripheral", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		if (_isRunning) ImGui::BeginDisabled();
		PushStyleCompact(1.0f, 0.0f);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		DrawRegs();
		ImGui::TableNextColumn();
		DrawStack();
		ImGui::TableNextColumn();
		DrawHardware();
		ImGui::TableNextColumn();
		DrawPeripheral();

		PopStyleCompact();
		if (_isRunning) ImGui::EndDisabled();
		
		ImGui::EndTable();
	}
	ImGui::PopStyleVar(2);
}

void dev::HardwareStatsWindow::UpdateData(const bool _isRunning)
{
	if (_isRunning) return;

	if (m_reqHardwareStatsReset) {
		m_ccLast = 0;
		m_reqHardwareStatsReset = false;
	}
	auto res = m_hardware.Request(Hardware::Req::GET_REGS);
	const auto& data = *res;

	uint64_t cc = data["cc"];
	auto ccDiff = cc - m_ccLast;
	m_ccLastRun = ccDiff == 0 ? m_ccLastRun : ccDiff;
	m_ccLast = cc;
	if (ccDiff == 0) return;

	// Regs
	CpuI8080::AF regAF = { data["af"] };
	Addr regBC = data["bc"];
	Addr regDE = data["de"];
	Addr regHL = data["hl"];
	Addr regSP = data["sp"];
	Addr regPC = data["pc"];

	// Flags

	bool cUpdated = regAF.c != m_cpuState.regs.psw.c;
	m_flagCColor = cUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	bool zUpdated = regAF.z != m_cpuState.regs.psw.z;
	m_flagZColor = zUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	bool pUpdated = regAF.p != m_cpuState.regs.psw.p;
	m_flagPColor = pUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	bool sUpdated = regAF.s != m_cpuState.regs.psw.s;
	m_flagSColor = sUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	bool acUpdated = regAF.ac != m_cpuState.regs.psw.ac;
	m_flagACColor = acUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;

	bool afUpdated = regAF.af.word != m_cpuState.regs.psw.af.word;
	m_regAFColor = afUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_cpuState.regs.psw = regAF;

	bool bcUpdated = regBC != m_cpuState.regs.bc.word;
	m_regBCColor = bcUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_cpuState.regs.bc.word = regBC;

	bool deUpdated = regDE != m_cpuState.regs.de.word;
	m_regDEColor = deUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_cpuState.regs.de.word = regDE;

	bool hlUpdated = regHL != m_cpuState.regs.hl.word;
	m_regHLColor = hlUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_cpuState.regs.hl.word = regHL;

	bool spUpdated = regSP != m_cpuState.regs.sp.word;
	m_regSPColor = spUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_cpuState.regs.sp.word = regSP;

	bool pcUpdated = regPC != m_cpuState.regs.pc.word;
	m_regPCColor = pcUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_cpuState.regs.pc.word = regPC;

	dev::CpuI8080::Int ints{ data["ints"] };

	// Stack
	Addr dataAddrN10 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP - 10 } })->at("data");
	Addr dataAddrN8 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP - 8 } })->at("data");
	Addr dataAddrN6 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP - 6 } })->at("data");
	Addr dataAddrN4 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP - 4 } })->at("data");
	Addr dataAddrN2 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP - 2 } })->at("data");
	Addr dataAddr0 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP } })->at("data");
	Addr dataAddrP2 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP + 2} })->at("data");
	Addr dataAddrP4 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP + 4 } })->at("data");
	Addr dataAddrP6 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP + 6 } })->at("data");
	Addr dataAddrP8 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP + 8 } })->at("data");
	Addr dataAddrP10 = m_hardware.Request(Hardware::Req::GET_WORD_STACK, { { "addr", regSP + 10 } })->at("data");

	m_dataAddrN10S = std::format("{:04X}", dataAddrN10);
	m_dataAddrN8S = std::format("{:04X}", dataAddrN8);
	m_dataAddrN6S = std::format("{:04X}", dataAddrN6);
	m_dataAddrN4S = std::format("{:04X}", dataAddrN4);
	m_dataAddrN2S = std::format("{:04X}", dataAddrN2);
	m_dataAddr0S = std::format("{:04X}", dataAddr0);
	m_dataAddrP2S = std::format("{:04X}", dataAddrP2);
	m_dataAddrP4S = std::format("{:04X}", dataAddrP4);
	m_dataAddrP6S = std::format("{:04X}", dataAddrP6);
	m_dataAddrP8S = std::format("{:04X}", dataAddrP8);
	m_dataAddrP10S = std::format("{:04X}", dataAddrP10);

	// Hardware
	res = m_hardware.Request(Hardware::Req::GET_DISPLAY_DATA);
	const auto& displayDataJ = *res;
	int rasterPixel = displayDataJ["rasterPixel"];
	int rasterLine = displayDataJ["rasterLine"];
	res = m_hardware.Request(Hardware::Req::GET_MEMORY_MAPPING);
	Memory::Mapping memMapping1 { res->at("mapping1") };
	Memory::Mapping memMapping2 { res->at("mapping2") };

	// update ram-disk 1
	m_mappingRamMode1S = "Off";
	if (memMapping1.data & Memory::MAPPING_RAM_MODE_MASK)
	{
		auto modeA = memMapping1.modeRamA ? "AC" : "--";
		auto mode8 = memMapping1.modeRam8 ? "8" : "-";
		auto modeE = memMapping1.modeRamE ? "E" : "-";
		m_mappingRamMode1S = std::format("{}{}{}", mode8, modeA, modeE);
	}
	m_mappingPageRam1S = std::to_string(memMapping1.pageRam);
	m_mappingModeStack1S = dev::BoolToStrC(memMapping1.modeStack, 2);
	m_mappingPageStack1S = std::to_string(memMapping1.pageStack);
	
	// update ram-disk 2
	m_mappingRamMode2S = "Off";
	if (memMapping2.data & Memory::MAPPING_RAM_MODE_MASK)
	{
		auto modeA = memMapping2.modeRamA ? "AC" : "--";
		auto mode8 = memMapping2.modeRam8 ? "8" : "-";
		auto modeE = memMapping2.modeRamE ? "E" : "-";
		m_mappingRamMode2S = std::format("{}{}{}", mode8, modeA, modeE);
	}
	m_mappingPageRam2S = std::to_string(memMapping2.pageRam);
	m_mappingModeStack2S = dev::BoolToStrC(memMapping2.modeStack, 2);
	m_mappingPageStack2S = std::to_string(memMapping2.pageStack);

	// update hardware
	m_ccS = std::to_string(cc);
	m_ccLastRunS = std::to_string(m_ccLastRun);
	m_rasterPixelS = std::to_string(rasterPixel);
	m_rasterLineS = std::to_string(rasterLine);

	res = m_hardware.Request(Hardware::Req::GET_PALETTE);
	const auto& paletteDataJ = *res;
	m_palette.low = paletteDataJ["low"];
	m_palette.hi = paletteDataJ["hi"];

	res = m_hardware.Request(Hardware::Req::GET_IO_PORTS);
	const auto& portsDataJ = *res;
	m_ports.data = portsDataJ["data"];

	m_cwS = std::to_string(m_ports.CW);
	m_portAS = std::to_string(m_ports.portA);
	m_portBS = std::to_string(m_ports.portB);
	m_portCS = std::to_string(m_ports.portC);
	m_cw2S = std::to_string(m_ports.CW2);
	m_portA2S = std::to_string(m_ports.portA2);
	m_portB2S = std::to_string(m_ports.portB2);
	m_portC2S = std::to_string(m_ports.portC2);
}

void dev::HardwareStatsWindow::UpdateDataRuntime()
{
	static int delay = 0;
	if (delay++ < 10) return;
	delay = 0;

	// FDC
	static const std::string diskNames[] = { "Drive A", "Drive B", "Drive C", "Drive D" };
	auto fdcInfo = *m_hardware.Request(Hardware::Req::GET_FDC_INFO);
	m_fdcDrive = diskNames[fdcInfo["drive"]];
	m_fdcSide = std::to_string(fdcInfo["side"].get<int>());
	m_fdcTrack = std::to_string(fdcInfo["track"].get<int>());
	m_fdcPosition = std::to_string(fdcInfo["position"].get<int>());
	m_fdcRwLen = std::to_string(fdcInfo["rwLen"].get<int>());
	m_fdcStats = std::format("Side {}\nTrack {}\nPosition {}\n R/W Len {}",
		m_fdcSide, m_fdcTrack, m_fdcPosition, m_fdcRwLen);

	for (int driveIdx = 0; driveIdx < Fdc1793::DRIVES_MAX; driveIdx++)
	{
		auto fddInfo = *m_hardware.Request(
			Hardware::Req::GET_FDD_INFO, { {"driveIdx", driveIdx} });
		size_t reads = fddInfo["reads"];
		size_t writes = fddInfo["writes"];
		m_fddPaths[driveIdx] = fddInfo["path"];

		m_fddStats[driveIdx] = fddInfo["mounted"] ? std::format("RW: {}/{}", reads, writes) : "dismounted";
	}

	// ruslat
	m_ruslatS = m_ruslat ? "(*)" : "( )";
}