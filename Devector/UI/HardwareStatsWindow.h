#pragma once

#include "Utils/Consts.h"
#include "Ui/BaseWindow.h"
#include "Core/Hardware.h"
#include "Core/CpuI8080.h"

namespace dev
{
	class HardwareStatsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 600;
		static constexpr int DEFAULT_WINDOW_H = 400;

		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;
		Hardware& m_hardware;
		bool& m_reqHardwareStatsReset;
		bool& m_ruslat;
		
		////////////////////
		// stats
		std::string m_regAFS;
		std::string m_regBCS;
		std::string m_regDES;
		std::string m_regHLS;
		std::string m_regSPS;
		std::string m_regPCS;

		dev::CpuI8080::AF m_regAF;
		dev::CpuI8080::Int m_ints;

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
		std::string m_rasterPixelS;
		std::string m_rasterLineS;
		std::string m_mappingRamMode1S;
		std::string m_mappingPageRam1S;
		std::string m_mappingModeStack1S;
		std::string m_mappingPageStack1S;
		std::string m_mappingRamMode2S;
		std::string m_mappingPageRam2S;
		std::string m_mappingModeStack2S;
		std::string m_mappingPageStack2S;
		std::string m_fdcDrive;
		std::string m_fdcSide;
		std::string m_fdcTrack;
		std::string m_fdcPosition;
		std::string m_fdcRwLen;
		std::string m_fdcStats;
		std::string m_fddStats[Fdc1793::DRIVES_MAX];
		std::string m_fddPaths[Fdc1793::DRIVES_MAX];
		std::string m_ruslatS;

		IO::Palette m_palette;
		IO::Ports m_ports;
		std::string m_cwS;
		std::string m_portAS;
		std::string m_portBS;
		std::string m_portCS;
		std::string m_cw2S;
		std::string m_portA2S;
		std::string m_portB2S;
		std::string m_portC2S;

		// stats end
		////////////////////

		void DrawStats(const bool _isRunning);
		void DrawRegs() const;
		void DrawStack() const;
		void DrawHardware() const;
		void DrawPeripheral() const;
		void UpdateData(const bool _isRunning);
		void UpdateDataRuntime();

	public:
		HardwareStatsWindow(Hardware& _hardware,
			const float* const _fontSizeP, const float* const _dpiScaleP, 
			bool& _reset, bool& _ruslat);
		
		void Update(bool& _visible);

	};

};