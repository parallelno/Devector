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
#include "Core/SoundAY8910.h"
#include "Core/Audio.h"
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
		SoundAY8910 m_ay;
		AYWrapper m_aywrapper;
		Audio m_audio;
		Fdc1793 m_fdc;

		enum class Status : int {
			RUN,
			STOP,
			EXIT
		};

	public:

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
			GET_BYTE_GLOBAL,
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
			GET_SCROLL_VERT,
			GET_STEP_OVER_ADDR,
			GET_IO_PORTS,
			GET_IO_PORTS_IN_DATA,
			GET_IO_PORTS_OUT_DATA,
			GET_IO_DISPLAY_MODE,
			GET_IO_PALETTE_COMMIT_TIME,
			SET_IO_PALETTE_COMMIT_TIME,
			GET_DISPLAY_BORDER_LEFT,
			SET_DISPLAY_BORDER_LEFT,
			GET_DISPLAY_IRQ_COMMIT_PXL,
			SET_DISPLAY_IRQ_COMMIT_PXL,
			SET_MEM,
			SET_CPU_SPEED,
			IS_MEMROM_ENABLED,
			KEY_HANDLING,
			LOAD_FDD,
			RESET_UPDATE_FDD,
			DEBUG_ATTACH,
			DEBUG_RESET,

			DEBUG_RECORDER_RESET,
			DEBUG_RECORDER_PLAY_FORWARD,
			DEBUG_RECORDER_PLAY_REVERSE,
			DEBUG_RECORDER_GET_STATE_RECORDED,
			DEBUG_RECORDER_GET_STATE_CURRENT,
			DEBUG_RECORDER_SERIALIZE,
			DEBUG_RECORDER_DESERIALIZE,

			DEBUG_BREAKPOINT_ADD,
			DEBUG_BREAKPOINT_DEL,
			DEBUG_BREAKPOINT_DEL_ALL,
			DEBUG_BREAKPOINT_GET_STATUS,
			DEBUG_BREAKPOINT_SET_STATUS,
			DEBUG_BREAKPOINT_ACTIVE,
			DEBUG_BREAKPOINT_DISABLE,
			DEBUG_BREAKPOINT_GET_ALL,
			DEBUG_BREAKPOINT_GET_UPDATES,

			DEBUG_WATCHPOINT_ADD,
			DEBUG_WATCHPOINT_DEL_ALL,
			DEBUG_WATCHPOINT_DEL,
			DEBUG_WATCHPOINT_GET_UPDATES,
			DEBUG_WATCHPOINT_GET_ALL,

		};

		using DebugFunc = std::function<bool(
			CpuI8080::State* _cpuState, Memory::State* _memState,
			IO::State* _ioState, Display::State* _displayState)>;

		using DebugReqHandlingFunc = std::function<nlohmann::json(Req _req, nlohmann::json _reqDataJ,
			CpuI8080::State* _cpuState, Memory::State* _memState,
			IO::State* _ioState, Display::State* _displayState)>;

		enum class ExecSpeed : int { _20PERCENT = 0, HALF, NORMAL, X2, MAX };


        Hardware(const std::wstring& _pathBootData);
		~Hardware();
		auto Request(const Req _req, const nlohmann::json& _dataJ = {}) -> Result <nlohmann::json>;
		auto GetFrame(const bool _vsync) -> const Display::FrameBuffer*;
		auto GetRam() const -> const Memory::Ram*;
		auto GetCpuState() -> const CpuI8080::State& { return m_cpu.GetState(); }
		auto GetMemState() -> const Memory::State& { return m_memory.GetState(); }
		auto GetIoState() -> const IO::State& { return m_io.GetState(); }

		void AttachDebugFuncs(DebugFunc _debugFunc, DebugReqHandlingFunc _debugReqHandlingFunc);


	private:
		DebugFunc Debug = nullptr;
		DebugReqHandlingFunc DebugReqHandling = nullptr;
		bool m_debugAttached = false;

		std::thread m_executionThread;
		std::thread m_reqHandlingThread;
		std::atomic<Status> m_status;
		TQueue <std::pair<Req, nlohmann::json>> m_reqs; // request
		TQueue <nlohmann::json> m_reqRes;				// request's result sent back 

		ExecSpeed m_execSpeed = ExecSpeed::NORMAL;
		std::chrono::microseconds m_execDelays[5] = { 99840us, 39936us, 19968us, 9984us, 10us };

		void Init();
		void Execution();
		bool ExecuteInstruction();
		void ExecuteFrameNoBreaks();
		void ReqHandling(const bool _waitReq = false);
		void Reset();
		void Restart();
		void Stop();
		void Run();
		auto GetRegs() const -> nlohmann::json;
		auto GetByteGlobal(const nlohmann::json _globalAddrJ) -> nlohmann::json;
		auto GetByte(const nlohmann::json _addrJ, const Memory::AddrSpace _addrSpace) -> nlohmann::json;
		auto Get3Bytes(const nlohmann::json _addrJ, const Memory::AddrSpace _addrSpace) -> nlohmann::json;
		auto GetWord(const nlohmann::json _addrJ, const Memory::AddrSpace _addrSpace) -> nlohmann::json;
		auto GetFddInfo(const int _driveIdx) -> Fdc1793::DiskInfo;
		auto GetFddImage(const int _driveIdx) -> const std::vector<uint8_t>;
		auto GetStepOverAddr() -> const Addr;
	};
}