#pragma once

#include <mutex>
#include <string>

#include "Utils/Types.h"
#include "Core/CpuI8080.h"
#include "Core/Memory.h"
#include "Core/Keyboard.h"
#include "Core/IO.h"
#include "Core/Display.h"
#include "Core/TimerI8253.h"
#include "Core/fdc1793.h"
#include "Utils/Utils.h"
#include "Utils/Result.h"
#include "Utils/TQueue.h"
#include "Utils/JsonUtils.h"
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>

namespace dev 
{
	class Hardware
	{
		CpuI8080 m_cpu;
		Memory m_memory;
		Keyboard m_keyboard;
		IO m_io;
		Display m_display;
		TimerI8253 m_timer;
		TimerWrapper m_timerWrapper;
		Fdc1793 m_fdc;

		enum class Status : int {
			RUN,
			STOP,
			EXIT
		};

	public:
		using CheckBreakFunc = std::function<bool(const CpuI8080::State& _cpuState, const Memory::State& _memState)>;
		using TraceLogUpdateFunc = std::function<bool(const CpuI8080::State& _cpuState, const Memory::State& _memState)>;

		enum class Req: int {
			NONE = 0,
			RUN,
			STOP,
			IS_RUNNING,
			EXIT,
			RESET,			// reboot the pc, enable the ROM
			RESTART,		// reboot the pc, disable the ROM
			EXECUTE_INSTR,
			EXECUTE_FRAME,
			EXECUTE_FRAME_NO_BREAKS,
			GET_CC,
			GET_REGS,
			GET_REG_PC,
			GET_BYTE_RAM,
			GET_THREE_BYTES_RAM,
			GET_WORD_STACK,
			GET_DISPLAY_DATA,
			GET_MEMORY_MAPPING,
			GET_GLOBAL_ADDR_RAM,
			GET_FDC_INFO,
			GET_FDD_INFO,
			GET_FDD_IMAGE,
			GET_RUSLAT_HISTORY,
			GET_PALETTE,
			GET_IO_PORTS,
			SET_MEM,
			IS_ROM_ENABLED,
			KEY_HANDLING,
			SCROLL_VERT,
			LOAD_FDD,
			RESET_UPDATE_FDD,
			SET_CPU_SPEED,
		};
		enum class ExecSpeed : int { _20PERCENT = 0, HALF, NORMAL, X2, MAX };

        Hardware(const std::wstring& _pathBootData);
		~Hardware();
		auto Request(const Req _req, const nlohmann::json& _dataJ = {}) -> Result <nlohmann::json>;
		auto GetFrame(const bool _vsync) -> const Display::FrameBuffer*;
		auto GetRam() const -> const Memory::Ram*;

		void AttachCheckBreak(CheckBreakFunc* _funcP);
		void AttachDebugOnReadInstr(CpuI8080::DebugOnReadInstrFunc* _funcP);
		void AttachDebugOnRead(CpuI8080::DebugOnReadFunc* _funcP);
		void AttachDebugOnWrite(CpuI8080::DebugOnWriteFunc* _funcP);
		void AttachTraceLogUpdate(TraceLogUpdateFunc* _funcP);

	private:
		std::atomic <CheckBreakFunc*> m_checkBreak;
		std::atomic <TraceLogUpdateFunc*> m_traceLogUpdate;
		std::thread m_executionThread;
		std::thread m_reqHandlingThread;
		std::atomic<Status> m_status;
		TQueue <std::pair<Req, nlohmann::json>> m_reqs; // a request type
		TQueue <nlohmann::json> m_reqRes;				// it's a result of a request sent back 

		ExecSpeed m_execSpeed = ExecSpeed::NORMAL;
		std::chrono::microseconds m_execDelays[5] = { 99840us, 39936us, 19968us, 9984us, 10us };

		void Init();
		void Execution();
		void ExecuteInstruction();
		void ExecuteFrameNoBreaks();
		void ReqHandling(const bool _waitReq = false);
		void Reset();
		void Restart();
		auto GetRegs() const -> nlohmann::json;
		auto GetByte(const nlohmann::json _addr, const Memory::AddrSpace _addrSpace) -> nlohmann::json;
		auto Get3Bytes(const nlohmann::json _addr, const Memory::AddrSpace _addrSpace) -> nlohmann::json;
		auto GetWord(const nlohmann::json _addr, const Memory::AddrSpace _addrSpace) -> nlohmann::json;
		auto GetFddInfo(const int _driveIdx) -> Fdc1793::DiskInfo;
		auto GetFddImage(const int _driveIdx) -> const std::vector<uint8_t>;
	};
}