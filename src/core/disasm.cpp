#include "core/disasm.h"
#include "core/disasm_i8080_cmds.h"
#include "core/disasm_z80_cmds.h"
#include "utils/str_utils.h"

// current ASM command syntax (i8080/z80/etc)
static const dev::Cmds* cmdsP = &cmds_i8080;

void dev::SetDisasmLang(const DisasmLang _lang)
{
	switch (_lang)
	{
	case DISASM_LANG_I8080:
		cmdsP = &cmds_i8080;
		break;
	case DISASM_LANG_Z80:
		cmdsP = &cmds_z80;
		break;
	}
}

bool dev::IsDisasmLangZ80()
{
	return cmdsP == &cmds_z80;
}

dev::Disasm::Disasm(Hardware& _hardware, DebugData& _debugData)
	:
	m_hardware(_hardware), m_debugData(_debugData)
{}

// UI thread
auto dev::Disasm::UpdateDisasm(
	const Addr _addr, size_t _linesNum, const int _instructionOffset)
-> void
{
	m_lines.clear();

	_linesNum = _linesNum == 0 ? DISASM_LINES_MAX :
		dev::Min(_linesNum, DISASM_LINES_MAX);

	// Find a new address that precedes the specified "addr" by the instructionOffset
	Addr addr = GetAddr(_addr, _instructionOffset);

	while (m_lines.size() < _linesNum)
	{
		// Add Comment
		auto commentP = m_debugData.GetComment(addr);
		if (commentP)
		{
			m_lines.emplace_back(DisasmLine{addr, *commentP});
			if (m_lines.size() >= DISASM_LINES_MAX) break;
		}

		// Add Labels
		auto labelsP = m_debugData.GetLabels(addr);
		if (labelsP)
		{
			m_lines.emplace_back(DisasmLine{addr, *labelsP});
			if (m_lines.size() >= DISASM_LINES_MAX) break;
		}

		// Add Code
		const auto& line = m_lines.emplace_back(DisasmLine{addr, m_hardware, m_debugData});
		if (m_lines.size() >= DISASM_LINES_MAX) break;

		addr += CpuI8080::GetInstrLen(line.opcode);
		addr &= 0xFFFF; // to loop back to the beginning of the page if addr > 0xFFFF
	}
}


// shifts the addr by _instructionsOffset instruction counter
// if _instructionsOffset=3, it returns the addr of a third instruction after _addr, and vice versa
#define MAX_ATTEMPTS 41 // max attemts to find an addr of an instruction before _addr
// check the perf of this func
auto dev::Disasm::GetAddr(const Addr _addr, const int _instructionOffset) const
-> Addr
{
	int instructions = dev::Abs(_instructionOffset);

	if (_instructionOffset > 0)
	{
		Addr addr = _addr;
		for (int i = 0; i < instructions; i++)
		{
			auto resOpcode = m_hardware.Request(
				Hardware::Req::GET_BYTE_RAM, { { "addr", addr } });

			uint8_t opcode = resOpcode->at("data");

			auto cmdLen = CpuI8080::GetInstrLen(opcode);
			addr = addr + cmdLen;
		}
		return addr;
	}
	else if (_instructionOffset < 0)
	{
		std::vector<Addr> possibleDisasmStartAddrs;

		int disasmStartAddr = _addr - instructions * CMD_BYTES_MAX;

		for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++)
		{
			int addr = disasmStartAddr;
			int currentInstruction = 0;

			while (addr < _addr && currentInstruction < instructions)
			{
				auto resOpcode = m_hardware.Request(
					Hardware::Req::GET_BYTE_RAM, { { "addr", addr } });
				uint8_t opcode = resOpcode->at("data");

				auto cmdLen = CpuI8080::GetInstrLen(opcode);
				addr = addr + cmdLen;
				currentInstruction++;
			}

			// if we reached the _addr address with counted instructions equals instructions
			if (addr == _addr && currentInstruction == instructions)
			{
				possibleDisasmStartAddrs.push_back((Addr)disasmStartAddr);
			}
			disasmStartAddr++;

			// return _addr if it fails to find a seaquence legal instructons
			if (disasmStartAddr + instructions > _addr)
			{
				break;
			}
		}
		if (possibleDisasmStartAddrs.empty()) {
			return Addr(_addr + _instructionOffset);
		}

		// get the best result basing on the execution counter
		for (const auto possibleDisasmStartAddr : possibleDisasmStartAddrs)
		{
			auto runs = m_debugData.GetMemRuns(possibleDisasmStartAddr);
			if (runs > 0) return possibleDisasmStartAddr;
		}
		return possibleDisasmStartAddrs[0];
	}

	return _addr;
}

dev::DisasmLine::DisasmLine(
	const Addr _addr, const std::vector<std::string>& _labels)
	: type(Type::LABELS), addr(_addr), cmdP(nullptr), opcode(0), imm(0)
{
	// use the first label as the main label
	label = _labels.at(0);

	// copy the rest of the labels to post_comment
	if (_labels.size() > 1)
	{
		post_comment = " ; ";
		post_comment += LabelsToComment(_labels, 1);
	}
};


auto dev::DisasmLine::LabelsToComment(
	const std::vector<std::string>& _labels, const size_t _start_idx) const
->std::string
{
	std::string comment;
	if (_labels.size() - _start_idx <= 0 ) return comment;

	for (int i = _start_idx;
		i < _labels.size() && i < MAX_LABELS_IN_LINE;
		i++)
	{
		comment += _labels.at(i);

		if (i != _labels.size() - 1) {
			comment += ", ";
		}
	}
	if (_labels.size() > MAX_LABELS_IN_LINE) {
		comment += " ...";
	}

	return comment;
};

dev::DisasmLine::DisasmLine(const Addr _addr,
	Hardware& _hardware, DebugData& _debugData)
	: type(Type::CODE), addr(_addr), opcode(0), imm(0)
{
	auto instr = Memory::Instr(_hardware.Request(
			Hardware::Req::GET_THREE_BYTES_RAM, {{"addr", _addr}}
			)->at("data"));

	InitInstr(addr, instr, _debugData, true);

	// init stats
	GlobalAddr globalAddr = _hardware.Request(
		Hardware::Req::GET_GLOBAL_ADDR_RAM, {{ "addr", addr}}
	)->at("data");

	auto runs = _debugData.GetMemRuns(globalAddr);
	auto reads = _debugData.GetMemReads(globalAddr);
	auto writes = _debugData.GetMemWrites(globalAddr);
	accessed = runs > 0 || reads > 0 || writes > 0;
	stats = std::format("{}, {}, {}", runs, reads, writes);
	// init breakpoint status
	breakpointStatus = _debugData.GetBreakpoints().GetStatus(globalAddr);
}


void dev::DisasmLine::InitInstr(
	const Addr _addr,
	const Memory::Instr _instr,
	const DebugData& _debugData,
	const bool _init_post_comment)
{
	type = Type::CODE;
	addr = _addr;
	opcode = _instr.opcode;
	cmdP = cmdsP->at(_instr.opcode);
	imm = InstrToImm(_instr, cmdP->imm_type);

	// Add consts
	if (cmdP->imm_type != CmdImmType::CMD_IT_NONE)
	{
		auto labelsP = _debugData.GetLabels(imm);
		auto constsP = _debugData.GetConsts(imm);

		imm_str = constsP ? constsP->at(0) :
			labelsP ? labelsP->at(0) :
			cmdP->imm_type == CmdImmType::CMD_IT_B0 ||
			cmdP->imm_type == CmdImmType::CMD_IT_B1 ? dev::Uint8ToStrC0x(imm) :
			dev::Uint16ToStrC0x(imm);

		imm_sub_type = constsP ? ImmSubType::CONST :
						labelsP ? ImmSubType::LABEL :
						ImmSubType::NONE;

		if (_init_post_comment && (labelsP || constsP)) {
			post_comment = constsP ? "; " : "";
			if (constsP) post_comment += LabelsToComment(*constsP, 1);

			if (labelsP) {
				post_comment += labelsP ? " (" : "";
				post_comment += LabelsToComment(*labelsP, 1);
				post_comment += labelsP ? ")" : "";
			}
		}
	}
}

auto dev::DisasmLine::InstrToImm(
	const Memory::Instr _instr, const CmdImmType _imm_type) const
-> Addr
{
	if (_imm_type == CMD_IT_B0)
		return _instr.opcode;
	else if (_imm_type == CMD_IT_B1)
		return _instr.dataL;

	return _instr.dataW;
}


// Disasm one command in a format:
// 1234\t12 34 56\tlxi sp,5634\tpsw=1234 bc=2345 de=3456 hl=4567 sp=5678
// Returns a pointer to the formatted string or nullptr if an error
auto dev::DisasmLine::PrintToBuffer(
	std::array<char, LINE_BUFF_LEN>& _buffer,
	const CpuI8080::State& _cpuState,
	const Memory::State& _memState,
	const Display::State& _displayState,
	const bool _printDisasm)
-> const char*
{
	int current_char_idx = 0;
	auto printed_chars = 0;
	_buffer.fill(0);

	// Instruction bytes as strings
	auto addr = _cpuState.regs.pc.word;
	auto globalAddr = _memState.GetGlobalAddr(addr, Memory::AddrSpace::RAM);
	auto opcode = _memState.ramP->at(globalAddr);
	auto dataL = _memState.ramP->at(globalAddr + 1);
	auto dataH = _memState.ramP->at(globalAddr + 2);
	auto dataW = (uint16_t)(dataL | (dataH << 8));

	auto instr_len = CpuI8080::GetInstrLen(opcode);

	// Print the Addr, intruction bytes
	printed_chars = std::snprintf(
		_buffer.data(), _buffer.size(),
		"%06X  %s %s %s  ",
		globalAddr,
		dev::Uint8ToStrC(opcode),
		instr_len > 1 ? dev::Uint8ToStrC(dataL) : "  ",
		instr_len > 2 ? dev::Uint8ToStrC(dataH) : "  ");
	// check for errors
	if (printed_chars < 0) { return nullptr; }
	current_char_idx += printed_chars;

	auto instr_pos = current_char_idx;

	// Print intruction
	if (_printDisasm)
	{
		auto cmdP = cmdsP->at(opcode);
		// Iterate over the tokens of the command and concatenate them into buffer
		for (int i = 0; i < cmdP->token_types.size(); i++)
		{
			if (cmdP->token_types[i] == CMD_TT_IMM)
			{
				auto imm_str = instr_len == 1 ?
					dev::Uint8ToStrC(dataL) :
					dev::Uint16ToStrC(dataW);

				printed_chars = std::snprintf(&_buffer[current_char_idx], LINE_BUFF_LEN - current_char_idx, imm_str);
			}
			else {
				printed_chars = std::snprintf(&_buffer[current_char_idx], LINE_BUFF_LEN - current_char_idx, cmdP->tokens[i]);
			}
			// check for errors
			if (printed_chars < 0) { return nullptr; }
			current_char_idx += printed_chars;
		}
	}

	// print spaces to line up the regs position
	auto spaces = current_char_idx == instr_pos ?
		0 :
		INSTR_LEN_MAX - (current_char_idx - instr_pos);
	std::fill_n(&_buffer[current_char_idx], spaces, ' ');
	current_char_idx += spaces;

	// print regs
	printed_chars = std::snprintf(
		&_buffer[current_char_idx], LINE_BUFF_LEN - current_char_idx,
		"A=%s BC=%s DE=%s HL=%s SP=%s S%s Z%s AC%s P%s CY%s CC=%012llu",
		dev::Uint8ToStrC(_cpuState.regs.psw.af.h),
		dev::Uint16ToStrC(_cpuState.regs.bc.word),
		dev::Uint16ToStrC(_cpuState.regs.de.word),
		dev::Uint16ToStrC(_cpuState.regs.hl.word),
		dev::Uint16ToStrC(_cpuState.regs.sp.word),
		_cpuState.regs.psw.s ? "1" : "0",
		_cpuState.regs.psw.z ? "1" : "0",
		_cpuState.regs.psw.ac ? "1" : "0",
		_cpuState.regs.psw.p ? "1" : "0",
		_cpuState.regs.psw.c ? "1" : "0",
		_cpuState.cc);

	// check for errors
	if (printed_chars < 0) { return nullptr; }
	current_char_idx += printed_chars;

	// print display state
	auto x = _displayState.update.framebufferIdx % Display::FRAME_W;
	auto y = _displayState.update.framebufferIdx / Display::FRAME_W;
	printed_chars = std::snprintf(
		&_buffer[current_char_idx], LINE_BUFF_LEN - current_char_idx,
		" scr=%03d/%03d scrl=%02X",
		x, y, _displayState.update.scrollIdx);
	// check for errors
	if (printed_chars < 0) { return nullptr; }
	current_char_idx += printed_chars;

	// add a new line & terminator
	current_char_idx = current_char_idx > LINE_BUFF_LEN - 2 ?
					LINE_BUFF_LEN - 2 :
					current_char_idx;
	_buffer[current_char_idx++] = '\n';
	_buffer[current_char_idx++] = '\0';

    return _buffer.data();
}


auto dev::Disasm::GetImmLinks() -> const ImmAddrLinks*
{
	m_immAddrLinks.clear();
	if (m_lines.size() == 0) return nullptr;


	Addr addrMin = m_lines.front().addr;
	Addr addrMax = m_lines.back().addr;

	// aggregate <Addr, LineIdx> pairs
	std::map<Addr, dev::Idx> immAddrPairs;
	dev::Idx lineIdx = 0;
	for (const auto& line : m_lines){
		immAddrPairs.emplace(line.addr, lineIdx++);
	}

	// generate links
	dev::Idx linkIdx = 0;
	lineIdx = -1;
	for (const auto& line : m_lines)
	{
		lineIdx++;

		if (line.type != DisasmLine::Type::CODE ||
			CpuI8080::GetInstrType(line.opcode) > CpuI8080::OPTYPE_JMP)
		{
			continue;
		}
		else if (line.imm < addrMin)
		{
			// adds links aiming to the address that is out of the visible range
			// m_immAddrLinks.emplace(lineIdx, IMM_LINK_UP, linkIdx++});
			continue;
		}
		else if (line.imm > addrMax)
		{
			// adds links aiming to the address that is out of the visible range
			// m_immAddrLinks.emplace(lineIdx, IMM_LINK_DOWN, linkIdx++});
			continue;
		}

		auto linkIdxIt = immAddrPairs.find(line.imm);
		if (linkIdxIt == immAddrPairs.end()) continue;

		m_immAddrLinks.emplace(lineIdx, Link{linkIdxIt->second, linkIdx++});
	}

	return &m_immAddrLinks;
}