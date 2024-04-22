#pragma once
#ifndef DEV_HARDWARE_STATS_WINDOW_H
#define DEV_HARDWARE_STATS_WINDOW_H

#include "Utils/Consts.h"
#include "Ui/BaseWindow.h"
#include "Core/Hardware.h"

namespace dev
{
	class HardwareStatsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;
		Hardware& m_hardware;
		bool& m_reset;
		
		////////////////////
		// stats
		std::string m_regAFS;
		std::string m_regBCS;
		std::string m_regDES;
		std::string m_regHLS;
		std::string m_regSPS;
		std::string m_regPCS;

		bool m_flagC = false;
		bool m_flagZ = false;
		bool m_flagP = false;
		bool m_flagS = false;
		bool m_flagAC = false;
		bool m_flagINTE = false;
		bool m_flagIFF = false;
		bool m_flagHLTA = false;

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
		std::string m_rasterPixel_rasterLineS;
		std::string m_mappingRamModeS;
		std::string m_mappingPageRamS;
		std::string m_mappingModeStackS;
		std::string m_mappingPageStackS;
		// stats end
		////////////////////

		void DrawStats(const bool _isRunning);
		void DrawRegs();
		void DrawFlags();
		void DrawStack();
		void DrawHardware();
		void UpdateData(const bool _isRunning);

	public:
		HardwareStatsWindow(Hardware& _hardware,
			const float* const _fontSizeP, const float* const _dpiScaleP, bool& _reset);
		
		void Update();

	};

};

#endif // !DEV_HARDWARE_STATS_WINDOW_H