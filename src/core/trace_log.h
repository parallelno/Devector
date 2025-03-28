#pragma once

#include <map>
#include <array>
#include <format>

#include "utils/types.h"
#include "core/breakpoint.h"
#include "core/cpu_i8080.h"
#include "core/memory.h"
#include "core/disasm.h"

namespace dev
{
	class TraceLog
	{
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

		std::array <Item, TRACE_LOG_SIZE> m_log;
		size_t m_logIdx = 0;
		Lines m_disasmLines;
		size_t m_disasmLinesLen = 0;

		TraceLog(const DebugData& _debugData);
		void AddCode(const Item& _item, Disasm::Line& _line);
		void Update(const CpuI8080::State& _cpuState, const Memory::State& _memState);
		auto GetDisasm(const size_t _lines, const uint8_t _filter) -> const Lines*;
		auto GetDisasmLen() -> const size_t { return m_disasmLinesLen; };
		void Reset();

	private:
		const DebugData& m_debugData;
	};


}