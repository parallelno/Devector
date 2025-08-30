#pragma once

#include <map>
#include <array>
#include <atomic>
#include <fstream>

#include "utils/types.h"
#include "core/breakpoint.h"
#include "core/cpu_i8080.h"
#include "core/memory.h"
#include "core/disasm.h"

namespace dev
{
	class TraceLog
	{
		static constexpr const char* TRACE_LOG_NAME = "trace_log";

	public:
		static const constexpr size_t TRACE_LOG_SIZE = 300000;
		static const constexpr int32_t EMPTY_ITEM = -1;

		struct Item
		{
			int32_t globalAddr = EMPTY_ITEM;
			uint8_t opcode = 0;
			CpuI8080::RegPair imm = 0; // immediate operand
		};

		using Lines = std::array<Disasm::Line, TRACE_LOG_SIZE>;

		// circular buffer. HW thread updates it
		std::array <Item, TRACE_LOG_SIZE> m_log;
		size_t m_logIdx = 0;
		// circular buffer. UI thread reads it
		Lines m_disasmLines;
		size_t m_disasmLinesLen = 0;

		TraceLog(const DebugData& _debugData);
		void AddCode(const Item& _item, Disasm::Line& _line);
		void Update(
			const CpuI8080::State& _cpuState, const Memory::State& _memState);
		auto GetDisasm(
			const size_t _lines, const uint8_t _filter) -> const Lines*;
		auto GetDisasmLen() -> const size_t { return m_disasmLinesLen; };
		void Reset();
		void SetSaveLog(bool _saveLog, const std::string& _path = {});
		static auto GetLogFilename() -> std::string;
		auto GetPath() const -> const std::string& { return m_saveLogPath; };

	private:
		static auto GetDefaultLogPath() -> std::string;
		void SaveLog(
			const CpuI8080::State& _cpuState, const Memory::State& _memState);
		void UpdateLogBuffer(
			const CpuI8080::State& _cpuState, const Memory::State& _memState);

		const DebugData& m_debugData;

		std::atomic<bool> m_saveLog = false;
		bool m_saveLogInited = false;
		std::string m_saveLogPath;
		std::ofstream m_logFile;
	};


}