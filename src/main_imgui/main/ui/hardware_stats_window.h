#pragma once

#include "utils/consts.h"
#include "ui/base_window.h"
#include "core/hardware.h"
#include "core/cpu_i8080.h"
#include "utils/imgui_utils.h"
#include "scheduler.h"

namespace dev
{
	class HardwareStatsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 600;
		static constexpr int DEFAULT_WINDOW_H = 400;

		int64_t m_ccLastRun = 0;
		Hardware& m_hardware;
		bool& m_ruslat;
		bool m_ruslatOld;
		int m_frameNum = 0;
		int m_displayMode = 0;

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
		int m_cpuRegM = 0;
		const ImVec4* m_regAColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regFColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regBColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regCColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regDColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regEColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regHColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regLColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regSPColor = &DASM_CLR_NUMBER;
		const ImVec4* m_regPCColor = &DASM_CLR_NUMBER;

		const ImVec4* m_flagCColor = &DASM_CLR_NUMBER;
		const ImVec4* m_flagZColor = &DASM_CLR_NUMBER;
		const ImVec4* m_flagPColor = &DASM_CLR_NUMBER;
		const ImVec4* m_flagSColor = &DASM_CLR_NUMBER;
		const ImVec4* m_flagACColor = &DASM_CLR_NUMBER;

		const ImVec4* m_frameNumColor = &DASM_CLR_NUMBER;
		const ImVec4* m_displayModeColor = &DASM_CLR_NUMBER;
		const ImVec4* m_scrollVColor = &DASM_CLR_NUMBER;
		const ImVec4* m_ruslatColor = &DASM_CLR_NUMBER;
		const ImVec4* m_inteColor = &DASM_CLR_NUMBER;
		const ImVec4* m_iffColor = &DASM_CLR_NUMBER;
		const ImVec4* m_hltaColor = &DASM_CLR_NUMBER;

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

		void Init();

		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;
		void DrawRegs() const;
		void DrawStack() const;
		void DrawHardware(const bool _isRunning) const;
		void DrawPeripheral() const;
		void DrawPortsDataProperty(const char* _name,
			const IO::PortsData& _portsData, const bool _isRunning,
			const PortsDataColors& _colors,
			const char* _hint = nullptr) const;

		void CallbackUpdateRegs(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data);
		void CallbackUpdateStack(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data);
		void CallbackUpdateHardware(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data);
		void CallbackUpdatePorts(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data);
		void CallbackUpdatePeripheral(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data);
		void CallbackUpdateFdc(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data);
		void CallbackUpdateTime(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data);
		void CallbackUpdatePalette(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data);

	public:

		HardwareStatsWindow(Hardware& _hardware,
			dev::Scheduler& _scheduler,
			bool* _visibleP, const float* const _dpiScaleP,
			bool& _ruslat);
	};

};