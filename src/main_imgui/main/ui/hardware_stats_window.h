#pragma once

#include "utils/consts.h"
#include "ui/base_window.h"
#include "core/hardware.h"
#include "core/cpu_i8080.h"
#include "utils/imgui_utils.h"

namespace dev
{
	class HardwareStatsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 600;
		static constexpr int DEFAULT_WINDOW_H = 400;

		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;
		Hardware& m_hardware;
		bool& m_ruslat;
		
		////////////////////
		// stats
		std::string m_dataAddrN10S;
		std::string m_dataAddrN8S;
		std::string m_dataAddrN6S;
		std::string m_dataAddrN4S;
		std::string m_dataAddrN2S;
		std::string m_dataAddr0S;
		std::string m_dataAddrP2S;
		std::string m_dataAddrP4S;
		std::string m_dataAddrP6S;
		std::string m_dataAddrP8S;
		std::string m_dataAddrP10S;

		std::string m_ccS;
		std::string m_ccLastRunS;
		std::string m_crtS;
		std::string m_frameCCS;
		std::string m_frameNumS;
		std::string m_mappingRamModeS;
		std::string m_mappingPageRamS;
		std::string m_mappingModeStackS;
		std::string m_mappingPageStackS;
		std::string m_ramdiskIdxS;
		std::string m_fdcDrive;
		std::string m_fdcSide;
		std::string m_fdcTrack;
		std::string m_fdcPosition;
		std::string m_fdcRwLen;
		std::string m_fdcStats;
		std::string m_fddStats[Fdc1793::DRIVES_MAX];
		std::string m_fddPaths[Fdc1793::DRIVES_MAX];
		std::string m_ruslatS;
		std::string m_displayModeS;

		CpuI8080::State m_cpuState;
		const ImVec4* m_regAFColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regBCColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regDEColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regHLColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regSPColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regPCColor = &DASM_CLR_NUMBER;

		const ImVec4* m_flagCColor = &DASM_CLR_NUMBER;
		const ImVec4* m_flagZColor = &DASM_CLR_NUMBER;
		const ImVec4* m_flagPColor = &DASM_CLR_NUMBER;
		const ImVec4* m_flagSColor = &DASM_CLR_NUMBER;
		const ImVec4* m_flagACColor = &DASM_CLR_NUMBER;

		IO::Palette m_palette;
		IO::PortsData m_portsInData;
		IO::PortsData m_portsOutData;
		using PortsDataColors = std::array<const ImVec4*, 256>;
		PortsDataColors m_portsInDataColor;
		PortsDataColors m_portsOutDataColor;

		std::string m_upTimeS;
		uint8_t m_scrollVert = 0;

		// stats end
		////////////////////

		void DrawStats(const bool _isRunning);
		void DrawRegs() const;
		void DrawStack() const;
		void DrawHardware(const bool _isRunning) const;
		void DrawPeripheral() const;
		void DrawPortsDataProperty(const char* _name, 
			const IO::PortsData& _portsData, const bool _isRunning,
			const PortsDataColors& _colors,
			const char* _hint = nullptr) const;
		void UpdateData(const bool _isRunning);
		void UpdateDataRuntime();
		void Init();
		void UpdateUpTime();

	public:
		HardwareStatsWindow(Hardware& _hardware,
			const float* const _dpiScaleP, 
			bool& _ruslat);
		
		void Update(bool& _visible, const bool _isRunning);

	};

};