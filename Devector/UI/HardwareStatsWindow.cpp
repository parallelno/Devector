#include <format>
#include "HardwareStatsWindow.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/StrUtils.h"

dev::HardwareStatsWindow::HardwareStatsWindow(Hardware& _hardware, 
		const float* const _fontSizeP, const float* const _dpiScaleP, 
		bool& _reset, const bool _restartOnLoadFdd)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_hardware(_hardware),
	m_reqHardwareStatsReset(_reset), m_restartOnLoadFdd(_restartOnLoadFdd)
{
	UpdateData(false);
}

void dev::HardwareStatsWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Hardware Stats", &open, ImGuiWindowFlags_NoCollapse);
	
	bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");
	UpdateData(isRunning);
	UpdateDataRuntime();
	DrawStats(isRunning);

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

		DrawProperty2("AF", m_regAFS);
		DrawProperty2("BC", m_regBCS);
		DrawProperty2("DE", m_regDES);
		DrawProperty2("HL", m_regHLS);
		DrawProperty2("SP", m_regSPS);
		DrawProperty2("PC", m_regPCS);

		ImGui::EndTable();
	}
}

void dev::HardwareStatsWindow::DrawFlags()
{
	static ImGuiTableFlags tableFlags =
		ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_NoPadInnerX |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("flags", 2, tableFlags))
	{
		ImGui::TableSetupColumn("flagsNames", ImGuiTableColumnFlags_WidthFixed, 45);

		DrawProperty2("C", dev::BoolToStr(m_flagC));
		DrawProperty2("Z", dev::BoolToStr(m_flagZ));
		DrawProperty2("P", dev::BoolToStr(m_flagP));
		DrawProperty2("S", dev::BoolToStr(m_flagS));
		DrawProperty2("AC", dev::BoolToStr(m_flagAC));
		DrawSeparator2("");
		DrawProperty2("INTE", dev::BoolToStr(m_flagINTE));
		DrawProperty2("IFF", dev::BoolToStr(m_flagIFF));
		DrawProperty2("HLTA", dev::BoolToStr(m_flagHLTA));

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
		DrawProperty2("-12", m_dataAddrN10S);
		DrawProperty2("-10", m_dataAddrN8S);
		DrawProperty2("-8", m_dataAddrN6S);
		DrawProperty2("-6", m_dataAddrN4S);
		DrawProperty2("-2", m_dataAddrN2S);
		DrawProperty2("SP", m_dataAddr0S);
		DrawProperty2("+2", m_dataAddrP2S);
		DrawProperty2("+4", m_dataAddrP4S);
		DrawProperty2("+6", m_dataAddrP6S);
		DrawProperty2("+8", m_dataAddrP8S);
		DrawProperty2("+10", m_dataAddrP10S);

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
		DrawProperty2("CPU Cicles", m_ccS);
		DrawProperty2("Last Run", m_ccLastRunS);
		DrawProperty2("CRT", m_rasterPixel_rasterLineS);
		DrawProperty2("Rus/Lat", m_ruslatS);

		// mapping
		DrawSeparator2("Ram-Disk:");
		DrawProperty2("RAM Mode", m_mappingRamModeS);
		DrawProperty2("RAM Page", m_mappingPageRamS);
		DrawProperty2("Stack Mode", m_mappingModeStackS);
		DrawProperty2("Stack Page", m_mappingPageStackS);

		// FDC
		DrawSeparator2("FDC:");
		static const std::string diskNames[] = { "Drive A", "Drive B", "Drive C", "Drive D" };
		DrawProperty2("Selected", m_fdcDrive, m_fdcStats);

		for (int i=0; i<Fdc1793::DRIVES_MAX; i++)
		{
			DrawProperty2(diskNames[i], m_fddStats[i], m_fddPaths[i]);
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
		ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 70);
		ImGui::TableSetupColumn("Stack", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("Hardware", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		if (_isRunning) ImGui::BeginDisabled();
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
	Addr regAF = data["af"];
	Addr regBC = data["bc"];
	Addr regDE = data["de"];
	Addr regHL = data["hl"];
	Addr regSP = data["sp"];
	Addr regPC = data["pc"];

	// Flags
	m_regAFS = std::format("{:04X}", regAF);
	m_regBCS = std::format("{:04X}", regBC);
	m_regDES = std::format("{:04X}", regDE);
	m_regHLS = std::format("{:04X}", regHL);
	m_regSPS = std::format("{:04X}", regSP);
	m_regPCS = std::format("{:04X}", regPC);

	m_flagC = data["flagC"];
	m_flagZ = data["flagZ"];
	m_flagP = data["flagP"];
	m_flagS = data["flagS"];
	m_flagAC = data["flagAC"];
	m_flagINTE = data["flagINTE"];
	m_flagIFF = data["flagIFF"];
	m_flagHLTA = data["flagHLTA"];

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
	const auto& displayData = *res;
	int rasterPixel = displayData["rasterPixel"];
	int rasterLine = displayData["rasterLine"];
	res = m_hardware.Request(Hardware::Req::GET_MEMORY_MODES);
	const auto& memoryModes = *res;
	bool mappingModeStack = memoryModes["mappingModeStack"];
	size_t mappingPageStack = memoryModes["mappingPageStack"];
	uint8_t mappingModeRam = memoryModes["mappingModeRam"];
	uint8_t mappingPageRam = memoryModes["mappingPageRam"];

	m_ccS = std::format("{}", cc);
	m_ccLastRunS = std::format("{}", m_ccLastRun);
	m_rasterPixel_rasterLineS = std::format("X: {:03} Y: {:03}", rasterPixel, rasterLine);

	m_mappingRamModeS = "Off";
	if ((mappingModeRam & Memory::MAPPING_RAM_MODE_MASK) > 0)
	{
		std::string mapping_ram_mode_a = (mappingModeRam & Memory::MAPPING_RAM_MODE_A000) > 0 ? "AC" : "--";
		std::string mapping_ram_mode_8 = (mappingModeRam & Memory::MAPPING_RAM_MODE_8000) > 0 ? "8" : "-";
		std::string mapping_ram_mode_e = (mappingModeRam & Memory::MAPPING_RAM_MODE_E000) > 0 ? "E" : "-";
		m_mappingRamModeS = mapping_ram_mode_8 + mapping_ram_mode_a + mapping_ram_mode_e;
	}

	m_mappingPageRamS = std::format("{}", mappingPageRam);
	m_mappingModeStackS = dev::BoolToStrC(mappingModeStack, true);
	m_mappingPageStackS = std::format("{}", mappingPageStack);
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
			Hardware::Req::GET_FDD_INFO, { {"_driveIdx", driveIdx} });
		size_t reads = fddInfo["reads"];
		size_t writes = fddInfo["writes"];
		m_fddPaths[driveIdx] = fddInfo["path"];

		m_fddStats[driveIdx] = fddInfo["mounted"] ? std::format("RW: {}/{}", reads, writes) : "dismounted";
	}

	// ruslat
	auto ruslatHistoryJ = *m_hardware.Request(Hardware::Req::GET_RUSLAT_HISTORY);
	auto m_ruslatHistory = ruslatHistoryJ["data"].get<uint32_t>();
	// auto press ruslat after loading fdd
	if (m_restartOnLoadFdd) {
		bool newRusLat = (m_ruslatHistory & 0b1000) != 0;

		if (newRusLat != m_ruslat) {
			if (m_rustLatSwitched++ > 2)
			{
				m_rustLatSwitched = 0;
				auto romEnabledJ = *m_hardware.Request(Hardware::Req::IS_ROM_ENABLED);
				if (romEnabledJ["data"]) {
					m_hardware.Request(Hardware::Req::RESTART);
				}
			}
		}
	}

	m_ruslat = (m_ruslatHistory & 0b1000) != 0;
	m_ruslatS = m_ruslat ? "(*)" : "( )";
}