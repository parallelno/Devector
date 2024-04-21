#pragma once
#ifndef DEV_DEBUGGER_H
#define DEV_DEBUGGER_H

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <array>
#include <format>

#include "Types.h"
#include "Hardware.h"
#include "Display.h"
#include "Breakpoint.h"
#include "Watchpoint.h"

namespace dev
{
	class Debugger
	{
	public:
		static const constexpr size_t TRACE_LOG_SIZE = 100000;

		struct DisasmLine 
		{
			enum class Type {
				COMMENT,
				LABELS,
				CODE
			};

			Type type;
			Addr addr;
			std::string addrS;
			std::string str; // labels, comments, code
			std::string consts; // labels that are assiciated with arguments of an operation
			std::string stats;
			uint64_t runs;
			uint64_t reads;
			uint64_t writes;
			const Breakpoint::Status breakpointStatus;

			DisasmLine(Type _type, Addr _addr, std::string _str,
				uint64_t _runs = UINT64_MAX, uint64_t _reads = UINT64_MAX, uint64_t _writes = UINT64_MAX, 
				std::string _consts = "", const Breakpoint::Status _breakpointStatus = Breakpoint::Status::DELETED)
				: 
				type(_type), addr(_addr), str(_str), 
				runs(_runs), reads(_reads), writes(_writes), 
				consts(_consts), breakpointStatus(_breakpointStatus)
			{
				if (type == Type::CODE) {
					addrS = std::format("0x{:04X}", _addr);
				}
				if (runs != UINT64_MAX)
				{
					std::string runsS = std::to_string(runs);
					std::string readsS = std::to_string(reads);
					std::string writesS = std::to_string(writes);
					stats = runsS + "," + readsS + "," + writesS;
				}
			};
			DisasmLine()
				: type(Type::CODE), addr(0), str(), stats(), 
				runs(), reads(), writes(), 
				consts(), breakpointStatus(Breakpoint::Status::DELETED)
			{};
		};
		using Disasm = std::vector<DisasmLine>;
		using Watchpoints = std::map<dev::Id, Watchpoint>;
		using Breakpoints = std::map<GlobalAddr, Breakpoint>;

		Debugger(Hardware& _hardware);
		~Debugger();
		void Init();
		void Reset();

		void ReadInstr(
			const GlobalAddr _globalAddr, const uint8_t _val, 
			const uint8_t _dataH, const uint8_t _dataL, const Addr _hl);
		void Read(const GlobalAddr _globalAddr, const uint8_t _val);
		void Write(const GlobalAddr _globalAddr, const uint8_t _val);

		auto GetDisasm(const Addr _addr, const size_t _lines, const int _instructionOffset) ->Disasm;

		void SetBreakpointStatus(const Addr _addr, const Breakpoint::Status _status);
		void AddBreakpoint(const Addr _addr,
			const uint8_t _mappingPages = Breakpoint::MAPPING_PAGES_ALL,
			const Breakpoint::Status _status = Breakpoint::Status::ACTIVE,
			const bool _autoDel = false, const std::string& _comment = "");
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

		auto GetTraceLog(const int _offset, const size_t _lines, const size_t _filter) -> std::string;
		void LoadDebugData(const std::wstring& _path);
		void ResetLabels();

		void ReqLoadRom(const std::wstring& _path);
		void ReqLoadRomLast();

	private:
		auto GetDisasmLine(const uint8_t _opcode, 
			const uint8_t _dataL, const uint8_t _dataH) const ->const std::string;
		auto GetDisasmLineDb(const uint8_t _data) const ->const std::string;
		auto GetAddr(const Addr _endAddr, const int _instructionOffset) const -> Addr;

		void TraceLogUpdate(const GlobalAddr _globalAddr, const uint8_t _opcode, 
			const uint8_t _dataH, const uint8_t _dataL, const Addr _hl);
		auto TraceLogNextLine(const int _idxOffset, const bool _reverse, const size_t _filter) const ->int;
		auto TraceLogNearestForwardLine(const size_t _idx, const size_t _filter) const ->int;

		static constexpr int LABEL_TYPE_LABEL		= 1 << 0;
		static constexpr int LABEL_TYPE_CONST		= 1 << 1;
		static constexpr int LABEL_TYPE_EXTERNAL	= 1 << 2;
		static constexpr int LABEL_TYPE_ALL			= LABEL_TYPE_LABEL | LABEL_TYPE_CONST | LABEL_TYPE_EXTERNAL;

		auto LabelsToStr(const Addr _addr, int _labelTypes) const -> const std::string;
		auto GetDisasmLabels(const Addr _addr) const -> const std::string;

		using MemStats = std::array<uint64_t, Memory::GLOBAL_MEMORY_LEN>;
		MemStats m_memRuns;
		MemStats m_memReads;
		MemStats m_memWrites;

		struct TraceLog
		{
			int64_t m_globalAddr;
			uint8_t m_opcode;
			uint8_t m_dataL;
			uint8_t m_dataH;

			auto ToStr() const->std::string;
			void Clear();
		};

		std::array <TraceLog, TRACE_LOG_SIZE> m_traceLog;
		size_t m_traceLogIdx = 0;
		int m_traceLogIdxViewOffset = 0;

		using AddrLabels = std::vector<std::string>;
		using Labels = std::map<GlobalAddr, AddrLabels>;
		Labels m_labels;		// labels
		Labels m_consts;		// labels used as constants only
		Labels m_externalLabels;// labels with a prefix "__" called externals and used in the code libraries in the ram-disk

		Hardware& m_hardware;

		std::mutex m_breakpointsMutex;
		std::mutex m_watchpointsMutex;
		Breakpoints m_breakpoints;
		Watchpoints m_watchpoints;
		bool m_wpBreak;

		Hardware::CheckBreakFunc m_checkBreakFunc;
		I8080::DebugOnReadInstrFunc m_debugOnReadInstrFunc;
		I8080::DebugOnReadFunc m_debugOnReadFunc;
		I8080::DebugOnWriteFunc m_debugOnWriteFunc;

		std::wstring m_pathLast;
	};
}
#endif // !DEV_DEBUGGER_H