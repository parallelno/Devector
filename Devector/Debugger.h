#pragma once
#ifndef DEV_DEBUGGER_H
#define DEV_DEBUGGER_H

#include <memory>
#include <mutex>
#include <map>
#include <vector>

#include "I8080.h"
#include "Memory.h"

namespace dev
{
	class Debugger
	{
	public:
		class Breakpoint
		{
		public:

			Breakpoint(const uint32_t _globalAddr, const bool _active = true)
				: m_globalAddr(_globalAddr), m_active(_active)
			{}
			auto Check() const -> const bool;
			auto IsActive() const -> const bool;
			void Print() const;

		private:
			uint32_t m_globalAddr;
			bool m_active;
		};

		class Watchpoint
		{
		public:
			enum class Access : size_t { R = 0, W, RW, COUNT };
			static constexpr const char* access_s[] = { "R-", "-W", "RW" };
			static constexpr size_t VAL_BYTE_SIZE = sizeof(uint8_t);
			static constexpr size_t VAL_WORD_SIZE = sizeof(uint16_t);
			static constexpr size_t VAL_MAX_SIZE = VAL_WORD_SIZE;

			enum class Condition : size_t { ANY = 0, EQU, LESS, GREATER, LESS_EQU, GREATER_EQU, NOT_EQU, COUNT };
			static constexpr const char* conditions_s[] = { "ANY", "==", "<", ">", "<=", ">=", "!=" };

			Watchpoint(const Access _access, const uint32_t _globalAddr, const Condition _cond, const uint16_t _value, const size_t _valueSize = VAL_BYTE_SIZE, const bool _active = true)
				: m_access(static_cast<Watchpoint::Access>((size_t)_access % (size_t)Access::COUNT)), m_globalAddr(_globalAddr), m_cond(static_cast<Debugger::Watchpoint::Condition>((size_t)_cond& (size_t)Condition::COUNT)),
				m_value(_value & 0xffff), m_valueSize(_valueSize), m_active(_active), m_breakL(false), m_breakH(false)
			{}
			auto Check(const Watchpoint::Access _access, const uint32_t _globalAddr, const uint8_t _value) -> const bool;
			auto IsActive() const -> const bool;
			auto GetGlobalAddr() const -> const size_t;
			auto CheckAddr(const uint32_t _globalAddr) const -> const bool;
			void Reset();
			void Print() const;

		private:
			Access m_access;
			uint32_t m_globalAddr;
			Condition m_cond;
			uint16_t m_value;
			size_t m_valueSize;
			bool m_active;
			bool m_breakL;
			bool m_breakH;
		};
		using Watchpoints = std::vector<Watchpoint>;
		static const constexpr size_t TRACE_LOG_SIZE = 100000;

		Debugger(I8080& _cpu, Memory& _memory);
		void Init();

		void Read(const uint32_t _globalAddr, Memory::AddrSpace _addrSpace, const uint8_t _val, const bool _isOpcode);
		void Write(const uint32_t _globalAddr, Memory::AddrSpace _addrSpace, const uint8_t _val);

		auto GetDisasm(const uint32_t _addr, const size_t _lines, const size_t _beforeAddrLines) const->std::vector<std::string>;

		void AddBreakpoint(const uint32_t _addr, const bool _active = true, const Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);
		void DelBreakpoint(const uint32_t _addr, const Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);
		bool CheckBreakpoints(const uint32_t _globalAddr);
		void PrintBreakpoints();

		void AddWatchpoint(const Watchpoint::Access _access, const uint32_t _addr, const Watchpoint::Condition _cond, const uint16_t _value, const size_t _valueSize = 1, const bool _active = true, const Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);
		void DelWatchpoint(const uint32_t _addr, const Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);
		bool CheckWatchpoint(const Watchpoint::Access _access, const uint32_t _globalAddr, const uint8_t _value);
		void ResetWatchpoints();
		void PrintWatchpoints();

		bool CheckBreak();

		auto GetTraceLog(const int _offset, const size_t _lines, const size_t _filter) -> std::string;
		void LoadLabels(const std::wstring& _path);

	private:
		auto GetDisasmLine(const uint32_t _addr, const uint8_t _opcode, const uint8_t _data_l, const uint8_t _data_h) const ->const std::string;
		auto GetDisasmLineDb(const uint32_t _addr, const uint8_t _data) const ->const std::string;
		auto GetCmdLen(const uint8_t _addr) const -> const size_t;
		auto GetAddr(const uint32_t _endAddr, const size_t _beforeAddrLines) const->size_t;
		auto WatchpointsFind(const uint32_t _globalAddr) -> Watchpoints::iterator;
		void WatchpointsErase(const uint32_t _globalAddr);

		void TraceLogUpdate(const uint32_t _globalAddr, const uint8_t _val);
		auto TraceLogNextLine(const int _idxOffset, const bool _reverse, const size_t _filter) const ->int;
		auto TraceLogNearestForwardLine(const size_t _idx, const size_t _filter) const ->int;

		uint64_t m_memRuns[Memory::GLOBAL_MEMORY_LEN];
		uint64_t m_memReads[Memory::GLOBAL_MEMORY_LEN];
		uint64_t m_memWrites[Memory::GLOBAL_MEMORY_LEN];

		struct TraceLog
		{
			int64_t m_globalAddr;
			uint8_t m_opcode;
			uint8_t m_dataL;
			uint8_t m_dataH;

			auto ToStr() const->std::string;
			void Clear();
		};

		TraceLog m_traceLog[TRACE_LOG_SIZE];
		size_t m_traceLogIdx = 0;
		int m_traceLogIdxViewOffset = 0;

		// labels names combined by their associated addr
		std::map<size_t, std::string> m_labels;
		// labels used as constants combined by their associated addr
		std::map<size_t, std::string> m_consts;
		// labels with a prefix "__" called externals and used in the code libraries in the ram-disk. they're combined by their associated addr
		std::map<size_t, std::string> m_externalLabels;

		I8080& m_cpu;
		Memory& m_memory;

		std::mutex m_breakpointsMutex;
		std::mutex m_watchpointsMutex;
		std::map<size_t, Breakpoint> m_breakpoints;
		Watchpoints m_watchpoints;
		bool m_wpBreak;
	};
}
#endif // !DEV_DEBUGGER_H