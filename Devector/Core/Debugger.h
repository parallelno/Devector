#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <array>
#include <format>

#include "Utils/Types.h"
#include "Core/Hardware.h"
#include "Core/Disasm.h"
#include "Core/DebugData.h"
#include "Core/Display.h"
#include "Core/Breakpoint.h"
#include "Core/Watchpoint.h"
#include "Core/TraceLog.h"
#include "Core/Recorder.h"
#include "Core/Breakpoints.h"
#include "Core/Watchpoints.h"

namespace dev
{
	class Debugger
	{
	public:
		static constexpr int LAST_RW_W = 32;
		static constexpr int LAST_RW_H = 32;
		static constexpr int LAST_RW_MAX = LAST_RW_W * LAST_RW_H; // should be squared because it is sent to GPU
		static constexpr uint32_t LAST_RW_NO_DATA = uint32_t(-1);
		using MemLastRW = std::array<uint32_t, Memory::GLOBAL_MEMORY_LEN>;
		using LastRWAddrs = std::array<uint32_t, LAST_RW_MAX>;

		Debugger(Hardware& _hardware);
		~Debugger();

		void Reset(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
			IO::State* _ioStateP, Display::State* _displayStateP);

		bool Debug(CpuI8080::State* _cpuStateP, Memory::State* _memStateP, 
			IO::State* _ioStateP, Display::State* _displayStateP);
		auto DebugReqHandling(Hardware::Req _req, nlohmann::json _reqDataJ, 
			CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
			IO::State* _ioStateP, Display::State* _displayStateP) -> nlohmann::json;

		void UpdateLastRW();
		auto GetLastRW() -> const MemLastRW* { return &m_memLastRW; }
		void UpdateDisasm(const Addr _addr, const size_t _lines, const int _instructionOffset);
		auto GetTraceLog() -> TraceLog& { return m_traceLog; };
		auto GetDebugData() -> DebugData& { return m_debugData; };
		auto GetDisasm() -> Disasm& { return m_disasm; };
		auto GetRecorder() -> Recorder& { return m_recorder; };

	private:

		Hardware& m_hardware;
		DebugData m_debugData;
		Disasm m_disasm;
		TraceLog m_traceLog;
		Recorder m_recorder;
		Breakpoints m_breakpoints;
		Watchpoints m_watchpoints;

		std::mutex m_lastRWMutex;
		LastRWAddrs m_lastReadsAddrs; // a circular buffer that contains addresses
		LastRWAddrs m_lastWritesAddrs; // ...
		LastRWAddrs m_lastReadsAddrsOld; // ... used to clean up m_memLastReads
		LastRWAddrs m_lastWritesAddrsOld; // ... ...
		LastRWAddrs m_lastRWAddrsOut; // used to sent the data out of this thread
		int m_lastReadsIdx = 0; // index to m_memLastReads, points to the least recently read. because it's a circular buffer, that element before it is the most recently read
		int m_lastWritesIdx = 0; // ...
		MemLastRW m_memLastRW; // low 2 bytes of each element contains the order of readings. 255 is the most recently read, 0 - the least recently read
								// high 2 bytes contains the order of writings. 255 is the most recently written, 0 - the least recently written
	};
}