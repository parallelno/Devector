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
#include "Core/Display.h"
#include "Core/Breakpoint.h"
#include "Core/Watchpoint.h"

namespace dev
{
	class Debugger
	{
	public:
		static const constexpr size_t TRACE_LOG_SIZE = 100000;
		static constexpr int LAST_RW_W = 32;
		static constexpr int LAST_RW_H = 32;
		static constexpr int LAST_RW_MAX = LAST_RW_W * LAST_RW_H; // should be squared because it is sent to GPU
		static constexpr uint32_t LAST_RW_NO_DATA = uint32_t(-1);
		using MemLastRW = std::array<uint32_t, Memory::GLOBAL_MEMORY_LEN>;
		using LastRWAddrs = std::array<uint32_t, LAST_RW_MAX>;

		Disasm disasm;
		using Watchpoints = std::unordered_map<dev::Id, Watchpoint>;
		using Breakpoints = std::unordered_map<GlobalAddr, Breakpoint>;

		Debugger(Hardware& _hardware);
		~Debugger();
		void Init();
		void Reset();

		void ReadInstr(
			const GlobalAddr _globalAddr, const uint8_t _val, 
			const uint8_t _dataH, const uint8_t _dataL, const Addr _hl);
		void Read(const GlobalAddr _globalAddr, const uint8_t _val);
		void Write(const GlobalAddr _globalAddr, const uint8_t _val);

		void SetBreakpointStatus(const Addr _addr, const Breakpoint::Status _status);
		void AddBreakpoint(const Addr _addr,
			const uint8_t _mappingPages = Breakpoint::MAPPING_PAGES_ALL,
			const Breakpoint::Status _status = Breakpoint::Status::ACTIVE,
			const bool _autoDel = false, 
			const Breakpoint::Operand _op = Breakpoint::Operand::A, 
			const Breakpoint::Condition _cond = Breakpoint::Condition::ANY, 
			const size_t _val = 0, const std::string& _comment = "");
		void DelBreakpoint(const Addr _addr);
		void DelBreakpoints();
		bool CheckBreakpoints(const Addr _addr, const uint8_t _mappingModeRam, const uint8_t _mappingPageRam);
		auto GetBreakpoints() -> const Breakpoints;
		auto GetBreakpointStatus(const Addr _addr) -> const Breakpoint::Status;

		void AddWatchpoint(const dev::Id _id, const Watchpoint::Access _access, 
			const GlobalAddr _globalAddr, const Watchpoint::Condition _cond, 
			const uint16_t _value, const Watchpoint::Type _type, const int _len = 1, 
			const bool _active = true, const std::string& _comment = "");
		void DelWatchpoint(const dev::Id _id);
		void DelWatchpoints();
		bool CheckWatchpoint(const Watchpoint::Access _access, const GlobalAddr _globalAddr, const uint8_t _value);
		void ResetWatchpoints();
		auto GetWatchpoints() -> const Watchpoints;

		bool CheckBreak(const Addr _addr, const uint8_t _mappingModeRam, const uint8_t _mappingPageRam);

		auto GetTraceLog(const int _offset, const size_t _lines, const size_t _filter) -> const Disasm::Lines*;

		auto GetLastRW() -> const MemLastRW*;
		void UpdateLastRW();
		void UpdateDisasm(const Addr _addr, const size_t _lines, const int _instructionOffset);

	private:
		void TraceLogUpdate(const GlobalAddr _globalAddr, const uint8_t _opcode, 
			const uint8_t _dataH, const uint8_t _dataL, const Addr _hl);
		auto TraceLogNextLine(const int _idxOffset, const bool _reverse, const size_t _filter) const ->int;
		auto TraceLogNearestForwardLine(const size_t _idx, const size_t _filter) const ->int;

		struct TraceLog
		{
			GlobalAddr m_globalAddr;
			uint8_t m_opcode;
			uint8_t m_dataL;
			uint8_t m_dataH;

			//auto ToStr() const -> std::string;
			//void Clear();
			Addr GetOperandAddr() const { return m_dataL | (m_dataH << 8); };
		};

		std::array <TraceLog, TRACE_LOG_SIZE> m_traceLog;
		size_t m_traceLogIdx = 0;
		int m_traceLogIdxViewOffset = 0;

		Hardware& m_hardware;

		std::mutex m_breakpointsMutex;
		std::mutex m_watchpointsMutex;
		Breakpoints m_breakpoints;
		Watchpoints m_watchpoints;
		bool m_wpBreak;

		Hardware::CheckBreakFunc m_checkBreakFunc;
		CpuI8080::DebugOnReadInstrFunc m_debugOnReadInstrFunc;
		CpuI8080::DebugOnReadFunc m_debugOnReadFunc;
		CpuI8080::DebugOnWriteFunc m_debugOnWriteFunc;

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