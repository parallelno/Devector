#include <format>
#include "ui/hardware_stats_window.h"
#include "utils/str_utils.h"

dev::HardwareStatsWindow::HardwareStatsWindow(Hardware& _hardware,
		dev::Scheduler& _scheduler,
		bool& _visible, const float* const _dpiScaleP,
		bool& _ruslat)
	:
	BaseWindow("Hardware Stats", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visible, _dpiScaleP),
	m_hardware(_hardware),
	m_ruslat(_ruslat)
{
	Init();
	UpdateData(false);
}

void dev::HardwareStatsWindow::Draw(const dev::Scheduler::Signals _signals)
{
	bool isRunning = dev::Scheduler::Signals::HW_RUNNING & _signals;

	UpdateData(isRunning);
	UpdateDataRuntime();
	DrawStats(isRunning);
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
		DrawProperty2RegPair("AF", Uint8ToStrC(m_cpuState.regs.psw.af.h), Uint8ToStrC(m_cpuState.regs.psw.af.l), nullptr, *m_regAColor, *m_regFColor);
		DrawProperty2RegPair("BC", Uint8ToStrC(m_cpuState.regs.bc.h), Uint8ToStrC(m_cpuState.regs.bc.l), nullptr, *m_regBColor, *m_regCColor);
		DrawProperty2RegPair("DE", Uint8ToStrC(m_cpuState.regs.de.h), Uint8ToStrC(m_cpuState.regs.de.l), nullptr, *m_regDColor, *m_regEColor);
		DrawProperty2RegPair("HL", Uint8ToStrC(m_cpuState.regs.hl.h), Uint8ToStrC(m_cpuState.regs.hl.l), nullptr, *m_regHColor, *m_regLColor);
		DrawProperty2("SP", Uint16ToStrC(m_cpuState.regs.sp.word), nullptr, *m_regSPColor);
		DrawProperty2("PC", Uint16ToStrC(m_cpuState.regs.pc.word), nullptr, *m_regPCColor);
		DrawProperty2(" M", Uint8ToStrC(m_cpuRegM), nullptr, *m_regPCColor);

		// flags
		ImGui::Dummy({1,8});
		DrawProperty2("C", dev::BoolToStrC(m_cpuState.regs.psw.c, 1), nullptr, *m_flagCColor);
		DrawProperty2("P", dev::BoolToStrC(m_cpuState.regs.psw.p, 1), nullptr, *m_flagPColor);
		DrawProperty2("AC", dev::BoolToStrC(m_cpuState.regs.psw.ac, 1), nullptr, *m_flagACColor);
		DrawProperty2("Z", dev::BoolToStrC(m_cpuState.regs.psw.z, 1), nullptr, *m_flagZColor);
		DrawProperty2("S", dev::BoolToStrC(m_cpuState.regs.psw.s, 1), nullptr, *m_flagSColor);


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
		DrawProperty2("-10", m_dataAddrN10S.c_str());
		DrawProperty2("-8", m_dataAddrN8S.c_str());
		DrawProperty2("-6", m_dataAddrN6S.c_str());
		DrawProperty2("-4", m_dataAddrN4S.c_str());
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

void dev::HardwareStatsWindow::DrawHardware(const bool _isRunning) const
{
	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_NoPadInnerX |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("hardware", 2, flags))
	{
		ImGui::TableSetupColumn("hwName", ImGuiTableColumnFlags_WidthFixed, 120);

		DrawProperty2("Up Time", m_upTimeS.c_str());
		DrawProperty2("CPU Cycles", m_ccS.c_str());
		DrawProperty2("Last Run", m_ccLastRunS.c_str());
		DrawProperty2("CRT X/Y", m_crtS.c_str());
		DrawProperty2("Frame CC", m_frameCCS.c_str());
		DrawProperty2("Frame Num", m_frameNumS.c_str(), nullptr, *m_frameNumColor);
		DrawProperty2("Display Mode", m_displayModeS.c_str(), nullptr, *m_displayModeColor);
		DrawProperty2("Scroll V", dev::Uint8ToStrC(m_scrollVert), nullptr, *m_scrollVColor);
		DrawProperty2("Rus/Lat", m_ruslatS.c_str(), nullptr, *m_ruslatColor);

		// interuption states
		ImGui::Dummy({ 1,8 });
		DrawProperty2("INTE", dev::BoolToStrC(m_cpuState.ints.inte), nullptr, *m_inteColor);
		DrawProperty2("IFF", dev::BoolToStrC(m_cpuState.ints.iff), nullptr, *m_iffColor);
		DrawProperty2("HLTA", dev::BoolToStrC(m_cpuState.ints.hlta), nullptr, *m_hltaColor);

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
		DrawPortsDataProperty("In", m_portsInData, _isRunning, m_portsInDataColor);
		DrawPortsDataProperty("Out", m_portsOutData, _isRunning, m_portsOutDataColor);

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

		// RAM Disk 1 mapping
		DrawSeparator2("RAM Disk:");
		DrawProperty2("Index", m_ramdiskIdxS.c_str());
		DrawProperty2("RAM Mode", m_mappingRamModeS.c_str());
		DrawProperty2("RAM Page", m_mappingPageRamS.c_str());
		DrawProperty2("Stack Mode", m_mappingModeStackS.c_str());
		DrawProperty2("Stack Page", m_mappingPageStackS.c_str());

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
		DrawHardware(_isRunning);
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

	auto res = m_hardware.Request(Hardware::Req::GET_REGS);
	const auto& data = *res;

	uint64_t cc = data["cc"];
	auto ccDiff = cc - m_ccLast;
	m_ccLastRun = ccDiff == 0 ? m_ccLastRun : ccDiff;
	m_ccLast = cc;
	if (ccDiff == 0) return;

	// Regs
	CpuI8080::AF regAF{ data["af"] };
	CpuI8080::RegPair regBC{ data["bc"] };
	CpuI8080::RegPair regDE{ data["de"] };
	CpuI8080::RegPair regHL{ data["hl"] };
	Addr regSP{ data["sp"] };
	Addr regPC{ data["pc"] };

	// Flags
	bool fcUpdated = regAF.c != m_cpuState.regs.psw.c;
	m_flagCColor = fcUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	bool fzUpdated = regAF.z != m_cpuState.regs.psw.z;
	m_flagZColor = fzUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	bool fpUpdated = regAF.p != m_cpuState.regs.psw.p;
	m_flagPColor = fpUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	bool fsUpdated = regAF.s != m_cpuState.regs.psw.s;
	m_flagSColor = fsUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	bool facUpdated = regAF.ac != m_cpuState.regs.psw.ac;
	m_flagACColor = facUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;

	bool aUpdated = regAF.af.h != m_cpuState.regs.psw.af.h;
	bool fUpdated = regAF.af.l != m_cpuState.regs.psw.af.l;
	m_regAColor = aUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_regFColor = fUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_cpuState.regs.psw = regAF;

	bool bUpdated = regBC.h != m_cpuState.regs.bc.h;
	bool cUpdated = regBC.l != m_cpuState.regs.bc.l;
	m_regBColor = bUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_regCColor = cUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_cpuState.regs.bc.word = regBC.word;

	bool dUpdated = regDE.h != m_cpuState.regs.de.h;
	bool eUpdated = regDE.l != m_cpuState.regs.de.l;
	m_regDColor = dUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_regEColor = eUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_cpuState.regs.de.word = regDE.word;

	bool hUpdated = regHL.h != m_cpuState.regs.hl.h;
	bool lUpdated = regHL.l != m_cpuState.regs.hl.l;
	m_regHColor = hUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_regLColor = lUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_cpuState.regs.hl.word = regHL.word;

	bool spUpdated = regSP != m_cpuState.regs.sp.word;
	m_regSPColor = spUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_cpuState.regs.sp.word = regSP;

	bool pcUpdated = regPC != m_cpuState.regs.pc.word;
	m_regPCColor = pcUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_cpuState.regs.pc.word = regPC;

	dev::CpuI8080::Int ints{ data["ints"] };
	bool inteUpdated = ints.inte != m_cpuState.ints.inte;
	m_inteColor = inteUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	bool iffUpdated = ints.iff != m_cpuState.ints.iff;
	m_iffColor = iffUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	bool hltaUpdated = ints.hlta != m_cpuState.ints.hlta;
	m_hltaColor = hltaUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_cpuState.ints = ints;

	m_cpuRegM = data["m"];

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
	int frameNum = displayDataJ["frameNum"];
	res = m_hardware.Request(Hardware::Req::GET_MEMORY_MAPPING);
	Memory::Mapping mapping { res->at("mapping") };
	int ramdiskIdx = res->at("ramdiskIdx");

	// update RAM Disk
	m_mappingRamModeS = mapping.RamModeToStr();
	m_mappingPageRamS = std::to_string(mapping.pageRam);
	m_mappingModeStackS = dev::BoolToStrC(mapping.modeStack, 2);
	m_mappingPageStackS = std::to_string(mapping.pageStack);
	m_ramdiskIdxS = std::to_string(ramdiskIdx + 1);

	// update hardware
	m_ccLastRunS = std::to_string(m_ccLastRun);
	m_crtS = std::format("{}/{}", rasterPixel, rasterLine);
	m_frameCCS = std::to_string((rasterPixel + rasterLine * Display::FRAME_W) / 4);

	bool frameNumUpdated = m_frameNum != frameNum;
	m_frameNumColor = frameNumUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_frameNum = frameNum;
	m_frameNumS = std::to_string(frameNum);

	res = m_hardware.Request(Hardware::Req::GET_IO_PALETTE);
	const auto& paletteDataJ = *res;
	m_palette.low = paletteDataJ["low"];
	m_palette.hi = paletteDataJ["hi"];

	// ports IN data
	res = m_hardware.Request(Hardware::Req::GET_IO_PORTS_IN_DATA);
	const auto& portsDataInJ = *res;
	IO::PortsData portdataIn;
	portdataIn.data0 = portsDataInJ["data0"];
	portdataIn.data1 = portsDataInJ["data1"];
	portdataIn.data2 = portsDataInJ["data2"];
	portdataIn.data3 = portsDataInJ["data3"];
	portdataIn.data4 = portsDataInJ["data4"];
	portdataIn.data5 = portsDataInJ["data5"];
	portdataIn.data6 = portsDataInJ["data6"];
	portdataIn.data7 = portsDataInJ["data7"];
	// check if updated, set the colors
	for (int i = 0; i < 256; i++)
	{
		bool updated = portdataIn.data[i] != m_portsInData.data[i];
		m_portsInDataColor[i] = updated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	}
	m_portsInData = portdataIn;

	// ports OUT data
	res = m_hardware.Request(Hardware::Req::GET_IO_PORTS_OUT_DATA);
	const auto& portsDataOutJ = *res;
	IO::PortsData portdataOut;
	portdataOut.data0 = portsDataOutJ["data0"];
	portdataOut.data1 = portsDataOutJ["data1"];
	portdataOut.data2 = portsDataOutJ["data2"];
	portdataOut.data3 = portsDataOutJ["data3"];
	portdataOut.data4 = portsDataOutJ["data4"];
	portdataOut.data5 = portsDataOutJ["data5"];
	portdataOut.data6 = portsDataOutJ["data6"];
	portdataOut.data7 = portsDataOutJ["data7"];
	// check if updated, set the colors
	for (int i = 0; i < 256; i++)
	{
		bool updated = portdataOut.data[i] != m_portsOutData.data[i];
		m_portsOutDataColor[i] = updated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	}
	m_portsOutData = portdataOut;

	// Vertical scroll
	auto scrollVert = m_hardware.Request(Hardware::Req::GET_SCROLL_VERT)->at("scrollVert");
	bool scrollVertUpdated = m_scrollVert != scrollVert;
	m_scrollVert = scrollVert;
	m_scrollVColor = scrollVertUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;

	// IO
	auto displayMode = m_hardware.Request(Hardware::Req::GET_IO_DISPLAY_MODE)->at("data");
	bool displayModeUpdated = m_displayMode != displayMode;
	m_displayMode = displayMode;
	m_displayModeColor = displayModeUpdated ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_displayModeS = m_displayMode ? "512" : "256";
}

void dev::HardwareStatsWindow::UpdateDataRuntime()
{
	static int delay = 0;
	if (delay++ < 10) return;
	delay = 0;

	// FDC
	static const std::string diskNames[] = { "Drive A", "Drive B", "Drive C", "Drive D" };
	auto fdcInfo = *m_hardware.Request(Hardware::Req::GET_FDC_INFO);
	m_fdcDrive = diskNames[fdcInfo["drive"].get<int>()];
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
	m_ruslatColor = m_ruslat != m_ruslatOld ? &CLR_NUM_UPDATED : &DASM_CLR_NUMBER;
	m_ruslatOld = m_ruslat;

	UpdateUpTime();
}

void dev::HardwareStatsWindow::DrawPortsDataProperty(const char* _name,
	const IO::PortsData& _portsData, const bool _isRunning, const PortsDataColors& _colors,
	const char* _hint) const
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	ImGui::Dummy(ImVec2(5.0f, 0.0f)); ImGui::SameLine();
	ImGui::Text(_isRunning ? "Stop emulation for info" : "Hover for info");

	if (ImGui::IsItemHovered())
	{
		for (int i=0; i<256; i++)
		{
			ImGui::BeginTooltip();
			ImGui::PushStyleColor(ImGuiCol_Text, *_colors[i]);
			ImGui::Text("%s, ", dev::Uint8ToStrC(_portsData.data[i]));
			ImGui::PopStyleColor();
			if (i % 16 != 15) ImGui::SameLine();
			ImGui::EndTooltip();
		}
	}

	if (_hint) {
		ImGui::SameLine();
		dev::DrawHelpMarker(_hint);
	}
}

void dev::HardwareStatsWindow::Init()
{
	m_portsInDataColor.fill(&DASM_CLR_NUMBER);
	m_portsOutDataColor.fill(&DASM_CLR_NUMBER);
	m_regAColor = &DASM_CLR_NUMBER;
	m_regFColor = &DASM_CLR_NUMBER;
	m_regBColor = &DASM_CLR_NUMBER;
	m_regCColor = &DASM_CLR_NUMBER;
	m_regDColor = &DASM_CLR_NUMBER;
	m_regEColor = &DASM_CLR_NUMBER;
	m_regHColor = &DASM_CLR_NUMBER;
	m_regLColor = &DASM_CLR_NUMBER;
	m_regSPColor = &DASM_CLR_NUMBER;
	m_regPCColor = &DASM_CLR_NUMBER;

	m_flagCColor = &DASM_CLR_NUMBER;
	m_flagZColor = &DASM_CLR_NUMBER;
	m_flagPColor = &DASM_CLR_NUMBER;
	m_flagSColor = &DASM_CLR_NUMBER;
	m_flagACColor = &DASM_CLR_NUMBER;

	m_frameNumColor = &DASM_CLR_NUMBER;
	m_displayModeColor = &DASM_CLR_NUMBER;
	m_scrollVColor = &DASM_CLR_NUMBER;
	m_ruslatColor = &DASM_CLR_NUMBER;
	m_inteColor = &DASM_CLR_NUMBER;
	m_iffColor = &DASM_CLR_NUMBER;
	m_hltaColor = &DASM_CLR_NUMBER;
}

void dev::HardwareStatsWindow::UpdateUpTime()
{
	// update the up time
	uint64_t cc = m_hardware.Request(Hardware::Req::GET_CC)->at("cc");
	m_ccS = std::to_string(cc);
	int sec = (int)(cc / CpuI8080::CLOCK);
	int hours = sec / 3600;
	int minutes = (sec % 3600) / 60;
	int seconds = sec % 60;
	m_upTimeS = std::format("{0:02}:{1:02}:{2:02}", hours, minutes, seconds);
}