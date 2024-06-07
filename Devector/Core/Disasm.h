#pragma once

#include <unordered_map>
#include <array>
#include <format>
#include <limits.h>

#include "Utils/Types.h"
#include "Core/Breakpoint.h"


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
	#define OPTYPE____	8

	auto AddrToAddrI16S(const Addr _addr) -> const char*;
	auto AddrToAddrI8S(const uint8_t _addr) -> const char*;
	auto GetMnemonic(const uint8_t _opcode) -> const char**;
	auto GetMnemonicLen(const uint8_t _opcode) -> uint8_t;
	auto GetMnemonicType(const uint8_t _opcode) -> const uint8_t*;
	auto GetImmediateType(const uint8_t _opcode) -> uint8_t;
	auto GetOpcodeType(const uint8_t _opcode) -> const uint8_t;

	struct Disasm
	{
		static constexpr size_t DISASM_LINES_MAX = 80;
		using AddrLabels = std::vector<std::string>;
		using Labels = std::unordered_map<GlobalAddr, AddrLabels>;
		using Comments = std::unordered_map<GlobalAddr, std::string>;
		using LineIdx = size_t;

		static constexpr int IMM_NO_LINK	= -1; // means no link from the immediate to the Addr
		static constexpr int IMM_LINK_UP	= INT_MIN; // means the link goes from the immediate above the first visible line
		static constexpr int IMM_LINK_DOWN	= INT_MAX; // means the link goes from the immediate below the last visible line

		struct Line
		{
			static constexpr size_t STATS_LEN = 256; 

			enum class Type {
				COMMENT,
				LABELS,
				CODE,
			};

			Type type = Type::CODE;
			Addr addr = 0;
			uint8_t opcode = 0;
			uint16_t imm = 0; // immediate operand
			char statsS[STATS_LEN] = {0}; // contains: runs, reads, writes
			const AddrLabels* labels	= nullptr;
			const AddrLabels* consts	= nullptr; // labels used as constants only
			const std::string* comment	= nullptr;
			bool accessed	= false; // no runs, reads, writes yet
			Breakpoint::Status breakpointStatus = Breakpoint::Status::DISABLED;

			void Init();
			auto GetStr() const -> std::string;

			inline const char* GetAddrS() const { return AddrToAddrI16S(addr); };
			inline const char* GetImmediateS() const;
			inline const char* GetFirstLabel() const { return labels ? labels->at(0).c_str() : nullptr; };
			inline const char* GetLabelConst() const { return labels ? labels->at(0).c_str() : consts ? consts->at(0).c_str() : nullptr; };
			inline const char* GetFirstConst() const { 
				return consts ? consts->at(0).c_str() : nullptr; 
			};
		};

		using Lines = std::array<Line, DISASM_LINES_MAX>;
		Lines lines;
		LineIdx lineIdx = 0; // the next avalable line
		LineIdx linesNum = 0; // the total number of lines

		// each element is associated with the disasm lines.
		// it represents a link between the immediate operand in this line 
		// and the another line with corresponding addr
		struct Link {
			int lineIdx = 0; // contains the index of the disasm line where the link goes. check IMM_NO_LINK, IMM_LINK_UP, IMM_LINK_DOWN
			uint8_t linkIdx = 0; // contains the index of the link
		};
		using ImmAddrLinks = std::array<Link, DISASM_LINES_MAX>;
		ImmAddrLinks immAddrLinks;
		size_t immAddrlinkNum = 0; // the total number of links between the immediate operand and the corresponding address
		
		void Init(LineIdx _linesNum);
		void AddLabes(const Addr _addr, const Labels& _labels);
		void AddComment(const Addr _addr, const Comments& _comments);

		auto AddDb(const Addr _addr, const uint8_t _data,
			const Labels& _consts,
			const uint64_t _runs, const uint64_t _reads, const uint64_t _writes,
			const Breakpoint::Status _breakpointStatus) -> Addr;

		auto AddCode(const Addr _addr, const uint32_t _cmd,
			const Labels& _labels, const Labels& _consts,
			const uint64_t _runs, const uint64_t _reads, const uint64_t _writes,
			const Breakpoint::Status _breakpointStatus) -> Addr;

		auto GetLines() -> const Lines* { return &lines; }
		auto GetLineNum() const -> LineIdx { return linesNum; }
		auto GetLineIdx() const -> LineIdx { return lineIdx; }
		auto GetImmLinks() -> const ImmAddrLinks*;
		bool IsDone() const { return lineIdx >= linesNum; }
		auto GetImmAddrlinkNum() const -> size_t { return immAddrlinkNum; }
	};
}