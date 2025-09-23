#pragma once

#include <list>
#include <string>

#include "utils/types.h"
#include "core/hardware.h"
#include "core/debug_data.h"
#include "core/breakpoint.h"

namespace dev
{
	enum CmdImmType {
		CMD_IT_NONE = 0, // no immediate operand
		CMD_IT_B0 = 1, // immediate byte, command offset = 0. Used to represent the data blobs such as DB 0x00, DB 0x01, DB 0x02, etc
		CMD_IT_B1 = 2, // immediate byte, command offset = 1
		CMD_IT_W1 = 3, // immediate word, command offset = 1
	};

	enum CmdTokenType {
		CMD_TT_NONE = 0,
		CMD_TT_CMD,      // command
		CMD_TT_REG,      // register
		CMD_TT_IMM,      // immediate operand
		CMD_TT_BIMM, // build-in immediate operand
		CMD_TT_COMMA, 	// comma between operands
		CMD_TT_COMMA_SPACE, // comma & spacebetween operands
		CMD_TT_SPACE,    // space
		CMD_TT_BRACKET_L, // left bracket
		CMD_TT_BRACKET_R, // right bracket
		CMD_TT_LABEL,    // label
		CMD_TT_CONST,    // constant
		CMD_TT_COMMENT   // comment
	};

	// max number of sub-strings in a command including mnemonic parts and immediate operand, labels, comments, consts, etc
	static constexpr int CMD_TOKENS_MAX = 7;

	struct Cmd{
		const char* tokens[CMD_TOKENS_MAX];
		const std::vector<CmdTokenType> token_types;
		const CmdImmType imm_type = CMD_IT_NONE;
	};

	struct DisasmLine
	{
		static constexpr size_t MAX_LABELS_IN_LINE = 10;
		static constexpr size_t LINE_BUFF_LEN = 127;

		enum class Type {
				COMMENT,
				LABELS,
				CODE,
			};
		// define what type of label is used for the immediate operand
		enum class ImmSubType {
				NONE = 0,
				CONST,
				LABEL
			};

		Type type = Type::CODE;
		Addr addr;
		const Cmd* cmdP;
		uint8_t opcode;
		Addr imm; // immediate operand
		std::string stats; // the addr stats: runs, reads, writes
		bool accessed = false; // no runs, reads, writes yet
		Breakpoint::Status breakpointStatus = Breakpoint::Status::DISABLED;

		std::string label; // the main label shown before ':'
		std::string imm_str; // used as a immediate operand substitution
		ImmSubType imm_sub_type = ImmSubType::NONE;
		std::string comment;
		std::string post_comment; // other labels and other consts associated with the addr or imm

		DisasmLine(Addr _addr = 0)
			: addr(_addr), cmdP(nullptr), opcode(0), imm(0)
		{};
		// for comments
		DisasmLine(const Addr _addr, const std::string& _comment)
			: type(Type::COMMENT), addr(_addr), comment(_comment),
			cmdP(nullptr), opcode(0), imm(0)
		{};
		// for labels
		DisasmLine(const Addr _addr, const std::vector<std::string>& _labels);
		// for code
		DisasmLine(const Addr _addr, Hardware& _hardware, DebugData& _debugData);

		void InitComment(const std::string& _comment);
		void InitLabels(const std::vector<std::string>& _labels);
		void InitCode(const Addr _addr,
					Hardware& _hardware, DebugData& _debugData);
		void InitInstr(const Addr _addr,
					   const Memory::Instr _instr,
					   const DebugData& _debugData,
					   const bool _init_post_comment);

		auto LabelsToComment(
			const std::vector<std::string>& _labels,
			const size_t _start_idx) const -> std::string;

		auto InstrToImm(const Memory::Instr _instr,
					    const CmdImmType _type) const -> Addr;

		static auto PrintToBuffer(
			std::array<char, LINE_BUFF_LEN>& _buffer,
			const GlobalAddr _globalAddr,
			const Memory::Instr _instr,
			const CpuI8080::Regs* _regsP = nullptr) -> const char*;
	};

	class Disasm
	{
	public:
		static constexpr size_t CMDS_MAX = 256;
		static constexpr size_t CMD_BYTES_MAX = 3;
		static constexpr size_t DISASM_LINES_MAX = 80;

		// indicates the link goes from the immediate above the first visible line
		static constexpr int IMM_LINK_UP = INT_MIN;
		// indicates the link goes from the immediate below the last visible line
		static constexpr int IMM_LINK_DOWN = INT_MAX;

		using Lines = std::list<DisasmLine>;

		// each element is associated with the disasm lines.
		// it represents a link between the immediate operand in this line
		// and the another line with corresponding addr
		struct Link {
			// contains the index of the disasm line where the link goes or
			// IMM_LINK_UP, IMM_LINK_DOWN
			dev::Idx endLineIdx = 0;
			// contains the index of the link
			dev::Idx linkIdx = 0;
		};
		using ImmAddrLinks = std::map<dev::Idx, Link>;


		void Init();
		auto GetLines() const -> const Lines& { return m_lines; }
		void AddCode(
			const Addr _addr,
			const uint8_t _opcode,
			const uint8_t _cmd_byte1,
			const uint8_t _cmd_byte2);
		void UpdateDisasm(
			const Addr _addr, size_t _linesNum, const int _instructionOffset);
		void Reset() { m_lines.clear(); };

		auto GetImmLinks() -> const ImmAddrLinks*;

		Disasm(Hardware& _hardware, DebugData& _debugData);

	private:
		auto GetAddr(const Addr _addr, const int _instructionOffset) const
			-> Addr;

		Lines m_lines;

		// The addr link is a UI element in the disasm window that represents a
		// line between the immediate operand and the corresponding address.
		ImmAddrLinks m_immAddrLinks;

		Hardware& m_hardware;
		DebugData& m_debugData;
	};
}