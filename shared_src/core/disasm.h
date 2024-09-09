#pragma once

#include <unordered_map>
#include <array>
#include <limits.h>

#include "utils/types.h"
#include "utils/str_utils.h"
#include "core/breakpoint.h"
#include "core/hardware.h"
#include "core/debug_data.h"

namespace dev
{
	// types of a mnemonic parts
	#define MNT_CMD		0 // command
	#define MNT_REG		1 // register
	#define MNT_IMM		2 // immediate operand

	// instruction immediate operand.
	#define CMD_IM_NONE 0 // no immediate operand
	#define CMD_IB_OFF0 1 // immediate byte, offset = 0
	#define CMD_IB_OFF1 2 // immediate byte, offset = 1
	#define CMD_IW_OFF1 3 // immediate word, offset = 1

	// opcode type
	#define OPTYPE_C__	0
	#define OPTYPE_CAL	1
	#define OPTYPE_J__	2
	#define OPTYPE_JMP	3
	#define OPTYPE_R__	4
	#define OPTYPE_RET	5
	#define OPTYPE_PCH	6
	#define OPTYPE_RST	7
	#define OPTYPE_ALL	8

	#define CMD_LEN_MAX 3

	auto GetMnemonic(const uint8_t _opcode) -> const char**;
	auto GetMnemonicLen(const uint8_t _opcode) -> uint8_t;
	auto GetMnemonicType(const uint8_t _opcode) -> const uint8_t*;
	auto GetImmediateType(const uint8_t _opcode) -> uint8_t;
	auto GetOpcodeType(const uint8_t _opcode) -> const uint8_t;
	auto GetCmdLen(const uint8_t _opcode) -> const uint8_t;

	class Disasm
	{
	public:
		static constexpr int IMM_NO_LINK = -1; // means no link from the immediate to the Addr
		static constexpr int IMM_LINK_UP = INT_MIN; // means the link goes from the immediate above the first visible line
		static constexpr int IMM_LINK_DOWN = INT_MAX; // means the link goes from the immediate below the last visible line
		static constexpr size_t DISASM_LINES_MAX = 80;

		using AddrLabels = std::vector<std::string>;
		using Labels = std::unordered_map<GlobalAddr, AddrLabels>;
		using Comments = std::unordered_map<GlobalAddr, std::string>;
		using LineIdx = size_t;

		void ResetConstsLabelsComments();

		struct Line
		{
			static constexpr size_t STATS_LEN = 128;

			enum class Type {
				COMMENT,
				LABELS,
				CODE,
			};

			Type type = Type::CODE;
			Addr addr = 0;
			uint8_t opcode = 0;
			uint16_t imm = 0; // immediate operand
			char statsS[STATS_LEN] = { 0 }; // contains: runs, reads, writes
			const AddrLabels* labels = nullptr;
			const AddrLabels* consts = nullptr; // labels used as constants or they point to data
			const std::string* comment = nullptr;
			bool accessed = false; // no runs, reads, writes yet
			Breakpoint::Status breakpointStatus = Breakpoint::Status::DISABLED;

			void Init();
			auto GetStr() const->std::string;

			inline auto GetAddrS() const -> const char* { return Uint16ToStrC0x(addr); };
			inline auto GetImmediateS() const -> const char*;
			inline auto GetFirstLabel() const -> const char* { return labels ? labels->at(0).c_str() : nullptr; };
			inline auto GetLabelConst() const -> const char* { return labels ? labels->at(0).c_str() : consts ? consts->at(0).c_str() : nullptr; };
			inline auto GetFirstConst() const -> const char* { return consts ? consts->at(0).c_str() : nullptr; };
		};

		using Lines = std::array<Line, DISASM_LINES_MAX>;

		// each element is associated with the disasm lines.
		// it represents a link between the immediate operand in this line 
		// and the another line with corresponding addr
		struct Link {
			int lineIdx = 0; // contains the index of the disasm line where the link goes. check IMM_NO_LINK, IMM_LINK_UP, IMM_LINK_DOWN
			uint8_t linkIdx = 0; // contains the index of the link
		};
		using ImmAddrLinks = std::array<Link, DISASM_LINES_MAX>;

		Disasm(Hardware& _hardware, DebugData& _debugData);
		void Init(LineIdx _linesNum);

		void AddLabes(const Addr _addr);
		void AddComment(const Addr _addr);
		auto AddCode(const Addr _addr, const uint32_t _cmd,
			const Breakpoint::Status _breakpointStatus) -> Addr;

		auto GetLines() -> const Lines** { return &m_linesP; };
		auto GetLineNum() const -> LineIdx { return m_linesNum; }
		auto GetLineIdx() const -> LineIdx { return m_lineIdx; }
		auto GetImmLinks() -> const ImmAddrLinks*;
		bool IsDone() const { return m_lineIdx >= m_linesNum; }
		auto GetImmAddrlinkNum() const -> size_t { return m_immAddrlinkNum; }

		auto GetAddr(const Addr _endAddr, const int _instructionOffset) const->Addr;
		void Reset();
		void SetUpdated() { m_linesP = &m_lines; };

		inline void MemRunsUpdate(const GlobalAddr _globalAddr) { m_memRuns[_globalAddr]++; };
		inline void MemReadsUpdate(const GlobalAddr _globalAddr) { m_memReads[_globalAddr]++; };
		inline void MemWritesUpdate(const GlobalAddr _globalAddr) { m_memWrites[_globalAddr]++; };

	private:

		Lines m_lines;
		const Lines* m_linesP = nullptr; // exposed pointer. it invalidates every time labels/commets/consts are updated
		LineIdx m_lineIdx = 0; // the next avalable line
		LineIdx m_linesNum = 0; // the total number of lines

		ImmAddrLinks m_immAddrLinks;
		size_t m_immAddrlinkNum = 0; // the total number of links between the immediate operand and the corresponding address
		Hardware& m_hardware;
		DebugData& m_debugData;
		
		using MemStats = std::array<uint64_t, Memory::GLOBAL_MEMORY_LEN>;
		MemStats m_memRuns;
		MemStats m_memReads;
		MemStats m_memWrites;
	};
}