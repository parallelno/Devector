#pragma once

#include <map>
#include <array>
#include <format>

#include "Utils/Types.h"
#include "Core/Breakpoint.h"


namespace dev
{
	/*void InitAddrsS();
	struct IniterAddrsS {
		IniterAddrsS();
	};
	static IniterAddrsS initerAddrsS;*/

	// types of a mnemonic parts
	#define MNT_CMD		0 // command
	#define MNT_REG		1 // register
	#define MNT_IMM		2 // immediate operand

	// instruction immediate operand.
	#define CMD_IM_NONE 0 // no immediate operand
	#define CMD_IB_OFF0 1 // immediate byte, offset = 0
	#define CMD_IB_OFF1 2 // immediate byte, offset = 1
	#define CMD_IW_OFF1 3 // immediate word, offset = 1

	auto AddrToAddrI16S(const Addr _addr) -> const char*;
	auto AddrToAddrI8S(const uint8_t _addr) -> const char*;
	auto GetMnemonic(const uint8_t _opcode) -> const char**;
	auto GetMnemonicLen(const uint8_t _opcode) -> uint8_t;
	auto GetMnemonicType(const uint8_t _opcode) -> const uint8_t*;
	auto GetImmediateType(const uint8_t _opcode) -> uint8_t;

	struct Disasm
	{
		static constexpr size_t DISASM_LINES_MAX = 80;
		using AddrLabels = std::vector<std::string>;
		using Labels = std::map<GlobalAddr, AddrLabels>;

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
			const AddrLabels* labels			= nullptr;
			const AddrLabels* consts	= nullptr; // labels used as constants only
			const AddrLabels* comments	= nullptr;
			bool accessed	= false; // no runs, reads, writes yet

			Breakpoint::Status breakpointStatus = Breakpoint::Status::DISABLED;
			auto GetStr() const -> std::string;

			inline const char* GetAddrS() const { return AddrToAddrI16S(addr); };
			inline const char* GetImmediateS() const;
			inline const char* GetLabel() const { return labels ? labels->at(0).c_str() : nullptr; };
			inline const char* GetLabelConst() const { return labels ? labels->at(0).c_str() : consts ? consts->at(0).c_str() : nullptr; };
			inline const char* GetConst() const { return consts ? consts->at(0).c_str() : nullptr; };
		};

		using Lines = std::array<Line, DISASM_LINES_MAX>;
		Lines lines;

		//Disasm() { InitAddrsS(); };
		
		auto AddLabes(const size_t _idx, const Addr _addr, const Labels& _labels) -> size_t;

		auto AddDb(const size_t _idx, Addr& _addr, const uint8_t _data,
			const Labels& _consts,
			const uint64_t _runs, const uint64_t _reads, const uint64_t _writes,
			const Breakpoint::Status _breakpointStatus) -> size_t;

		auto AddCode(const size_t _idx, Addr& _addr, const uint32_t _cmd,
			const Labels& _labels, const Labels& _consts,
			const uint64_t _runs, const uint64_t _reads, const uint64_t _writes,
			const Breakpoint::Status _breakpointStatus) -> size_t;

		auto GetLines() -> const Lines* { return &lines; }
	};
}