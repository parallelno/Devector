#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <array>
#include <format>

#include "utils/types.h"
#include "core/hardware.h"
//#include "core/disasm.h"
#include "core/disasm.h"
#include "core/debug_data.h"
#include "core/display.h"
#include "core/breakpoint.h"
#include "core/watchpoint.h"
#include "core/trace_log.h"
#include "core/recorder.h"

namespace dev
{
	class Debugger
	{
	public:
		static constexpr int LAST_RW_W = 32;
		static constexpr int LAST_RW_H = 32;
		static constexpr int LAST_RW_MAX = LAST_RW_W * LAST_RW_H; // should be squared because it is sent to GPU
		static constexpr uint32_t LAST_RW_NO_DATA = uint32_t(-1);
		using MemLastRW = std::array<uint32_t, Memory::MEMORY_GLOBAL_LEN>;
		using LastRWAddrs = std::array<uint32_t, LAST_RW_MAX>;

		Debugger(Hardware& _hardware, const int _recordFrames);
		~Debugger();

		void Reset(bool _resetRecorder,
			CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
			IO::State* _ioStateP, Display::State* _displayStateP);

		bool Debug(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
			IO::State* _ioStateP, Display::State* _displayStateP);
		auto DebugReqHandling(Hardware::Req _req, nlohmann::json _reqDataJ,
			CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
			IO::State* _ioStateP, Display::State* _displayStateP) -> nlohmann::json;

		void UpdateLastRW();
		auto GetLastRW() -> const MemLastRW* { return &m_memLastRW; }
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