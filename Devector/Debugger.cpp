#include "Debugger.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>
#include "Utils/StringUtils.h"
#include "Utils/Utils.h"

dev::Debugger::Debugger(Hardware& _hardware)
	:
	m_hardware(_hardware),
	m_wpBreak(false),
	m_traceLog(),
	m_pathLast()
{
    Init();
}

void dev::Debugger::Init()
{
	m_checkBreakFunc = std::bind(&Debugger::CheckBreak, this, std::placeholders::_1);
	m_debugOnReadInstrFunc = std::bind(&Debugger::ReadInstr, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
	m_debugOnReadFunc = std::bind(&Debugger::Read, this, std::placeholders::_1, std::placeholders::_2);
	m_debugOnWriteFunc = std::bind(&Debugger::Write, this, std::placeholders::_1, std::placeholders::_2);

	m_hardware.AttachCheckBreak( &m_checkBreakFunc );
	m_hardware.AttachDebugOnReadInstr( &m_debugOnReadInstrFunc );
	m_hardware.AttachDebugOnRead( &m_debugOnReadFunc );
	m_hardware.AttachDebugOnWrite( &m_debugOnWriteFunc );

	Reset();

	m_breakpoints.clear();
	m_watchpoints.clear();

	m_hardware.Request(Hardware::Req::RUN);
}

dev::Debugger::~Debugger()
{
	m_hardware.AttachCheckBreak(nullptr);
	m_hardware.AttachDebugOnReadInstr(nullptr);
	m_hardware.AttachDebugOnRead(nullptr);
	m_hardware.AttachDebugOnWrite(nullptr);
}

void dev::Debugger::Reset()
{
	m_memRuns.fill(0);
	m_memReads.fill(0);
	m_memWrites.fill(0);

	for (size_t i = 0; i < TRACE_LOG_SIZE; i++)
	{
		m_traceLog[i].Clear();
	}
	m_traceLogIdx = 0;
	m_traceLogIdxViewOffset = 0;
}

// a hardware thread
void dev::Debugger::ReadInstr(
	const GlobalAddr _globalAddr, const uint8_t _val, const uint8_t _dataH, const uint8_t _dataL, const Addr _hl)
{
	m_memRuns[_globalAddr]++;
	TraceLogUpdate(_globalAddr, _val, _dataH, _dataL, _hl);
}

// a hardware thread
void dev::Debugger::Read(
    const GlobalAddr _globalAddr, const uint8_t _val)
{
	m_memReads[_globalAddr]++;
    m_wpBreak |= CheckWatchpoint(Watchpoint::Access::R, _globalAddr, _val);
}
// a hardware thread
void dev::Debugger::Write(const GlobalAddr _globalAddr, const uint8_t _val)
{
    m_memWrites[_globalAddr]++;
    m_wpBreak |= CheckWatchpoint(Watchpoint::Access::W, _globalAddr, _val);
}


//////////////////////////////////////////////////////////////
//
// Disasm
//
//////////////////////////////////////////////////////////////

static const char* mnemonics[0x100] =
{
	"nop",	   "lxi b",  "stax b", "inx b",  "inr b", "dcr b", "mvi b", "rlc", "db 0x08", "dad b",  "ldax b", "dcx b",  "inr c", "dcr c", "mvi c", "rrc",
	"db 0x10", "lxi d",  "stax d", "inx d",  "inr d", "dcr d", "mvi d", "ral", "db 0x18", "dad d",  "ldax d", "dcx d",  "inr e", "dcr e", "mvi e", "rar",
	"db 0x20", "lxi h",  "shld",   "inx h",  "inr h", "dcr h", "mvi h", "daa", "db 0x28", "dad h",  "lhld",   "dcx h",  "inr l", "dcr l", "mvi l", "cma",
	"db 0x30", "lxi sp", "sta",    "inx sp", "inr m", "dcr m", "mvi m", "stc", "db 0x38", "dad sp", "lda",    "dcx sp", "inr a", "dcr a", "mvi a", "cmc",

	"mov b b", "mov b c", "mov b d", "mov b e", "mov b h", "mov b l", "mov b m", "mov b a", "mov c b", "mov c c", "mov c d", "mov c e", "mov c h", "mov c l", "mov c m", "mov c a",
	"mov d b", "mov d c", "mov d d", "mov d e", "mov d h", "mov d l", "mov d m", "mov d a", "mov e b", "mov e c", "mov e d", "mov e e", "mov e h", "mov e l", "mov e m", "mov e a",
	"mov h b", "mov h c", "mov h d", "mov h e", "mov h h", "mov h l", "mov h m", "mov h a", "mov l b", "mov l c", "mov l d", "mov l e", "mov l h", "mov l l", "mov l m", "mov l a",
	"mov m b", "mov m c", "mov m d", "mov m e", "mov m h", "mov m l", "hlt",     "mov m a", "mov a b", "mov a c", "mov a d", "mov a e", "mov a h", "mov a l", "mov a m", "mov a a",

	"add b", "add c", "add d", "add e", "add h", "add l", "add m", "add a", "adc b", "adc c", "adc d", "adc e", "adc h", "adc l", "adc m", "adc a",
	"sub b", "sub c", "sub d", "sub e", "sub h", "sub l", "sub m", "sub a", "sbb b", "sbb c", "sbb d", "sbb e", "sbb h", "sbb l", "sbb m", "sbb a",
	"ana b", "ana c", "ana d", "ana e", "ana h", "ana l", "ana m", "ana a", "xra b", "xra c", "xra d", "xra e", "xra h", "xra l", "xra m", "xra a",
	"ora b", "ora c", "ora d", "ora e", "ora h", "ora l", "ora m", "ora a", "cmp b", "cmp c", "cmp d", "cmp e", "cmp h", "cmp l", "cmp m", "cmp a",

	"rnz", "pop b",   "jnz", "jmp",  "cnz", "push b",   "adi", "rst 0x0", "rz",  "ret",     "jz",  "db 0xCB", "cz",  "call",    "aci", "rst 0x1",
	"rnc", "pop d",   "jnc", "out",  "cnc", "push d",   "sui", "rst 0x2", "rc",  "db 0xD9", "jc",  "in",      "cc",  "db 0xDD", "sbi", "rst 0x3",
	"rpo", "pop h",   "jpo", "xthl", "cpo", "push h",   "ani", "rst 0x4", "rpe", "pchl",    "jpe", "xchg",    "cpe", "db 0xED", "xri", "rst 0x5",
	"rp",  "pop PSW", "jp",  "di",   "cp",  "push PSW", "ori", "rst 0x6", "rm",  "sphl",    "jm",  "ei",      "cm",  "db 0xFD", "cpi", "rst 0x7"
};

// CAPITAL NAMING
/*
	"NOP",	   "LXI B",  "STAX B", "INX B",  "INR B", "DCR B", "MVI B", "RLC", "DB 0x08", "DAD B",  "LDAX B", "DCX B",  "INR C", "DCR C", "MVI C", "RRC",
	"DB 0x10", "LXI D",  "STAX D", "INX D",  "INR D", "DCR D", "MVI D", "RAL", "DB 0x18", "DAD D",  "LDAX D", "DCX D",  "INR E", "DCR E", "MVI E", "RAR",
	"DB 0x20", "LXI H",  "SHLD",   "INX H",  "INR H", "DCR H", "MVI H", "DAA", "DB 0x28", "DAD H",  "LHLD",   "DCX H",  "INR L", "DCR L", "MVI L", "CMA",
	"DB 0x30", "LXI SP", "STA",    "INX SP", "INR M", "DCR M", "MVI M", "STC", "DB 0x38", "DAD SP", "LDA",    "DCX SP", "INR A", "DCR A", "MVI A", "CMC",

	"MOV B B", "MOV B C", "MOV B D", "MOV B E", "MOV B H", "MOV B L", "MOV B M", "MOV B A", "MOV C B", "MOV C C", "MOV C D", "MOV C E", "MOV C H", "MOV C L", "MOV C M", "MOV C A",
	"MOV D B", "MOV D C", "MOV D D", "MOV D E", "MOV D H", "MOV D L", "MOV D M", "MOV D A", "MOV E B", "MOV E C", "MOV E D", "MOV E E", "MOV E H", "MOV E L", "MOV E M", "MOV E A",
	"MOV H B", "MOV H C", "MOV H D", "MOV H E", "MOV H H", "MOV H L", "MOV H M", "MOV H A", "MOV L B", "MOV L C", "MOV L D", "MOV L E", "MOV L H", "MOV L L", "MOV L M", "MOV L A",
	"MOV M B", "MOV M C", "MOV M D", "MOV M E", "MOV M H", "MOV M L", "HLT",     "MOV M A", "MOV A B", "MOV A C", "MOV A D", "MOV A E", "MOV A H", "MOV A L", "MOV A M", "MOV A A",

	"ADD B", "ADD C", "ADD D", "ADD E", "ADD H", "ADD L", "ADD M", "ADD A", "ADC B", "ADC C", "ADC D", "ADC E", "ADC H", "ADC L", "ADC M", "ADC A",
	"SUB B", "SUB C", "SUB D", "SUB E", "SUB H", "SUB L", "SUB M", "SUB A", "SBB B", "SBB C", "SBB D", "SBB E", "SBB H", "SBB L", "SBB M", "SBB A",
	"ANA B", "ANA C", "ANA D", "ANA E", "ANA H", "ANA L", "ANA M", "ANA A", "XRA B", "XRA C", "XRA D", "XRA E", "XRA H", "XRA L", "XRA M", "XRA A",
	"ORA B", "ORA C", "ORA D", "ORA E", "ORA H", "ORA L", "ORA M", "ORA A", "CMP B", "CMP C", "CMP D", "CMP E", "CMP H", "CMP L", "CMP M", "CMP A",

	"RNZ", "POP B",   "JNZ", "JMP",  "CNZ", "PUSH B",   "ADI", "RST 0x0", "RZ",  "RET",     "JZ",  "DB 0xCB", "CZ",  "CALL",    "ACI", "RST 0x1",
	"RNC", "POP D",   "JNC", "OUT",  "CNC", "PUSH D",   "SUI", "RST 0x2", "RC",  "DB 0xD9", "JC",  "IN",      "CC",  "DB 0xDD", "SBI", "RST 0x3",
	"RPO", "POP H",   "JPO", "XTHL", "CPO", "PUSH H",   "ANI", "RST 0x4", "RPE", "PCHL",    "JPE", "XCHG",    "CPE", "DB 0xED", "XRI", "RST 0x5",
	"RP",  "POP PSW", "JP",  "DI",   "CP",  "PUSH PSW", "ORI", "RST 0x6", "RM",  "SPHL",    "JM",  "EI",      "CM",  "DB 0xFD", "CPI", "RST 0x7"
*/

// define the maximum number of bytes in a command
#define CMD_LEN_MAX 3

// array containing lengths of commands, indexed by an opcode
static const uint8_t cmd_lens[0x100] =
{
	1,3,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,3,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,3,3,1,1,1,2,1,1,1,3,1,1,1,2,1,
	1,3,3,1,1,1,2,1,1,1,3,1,1,1,2,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,3,3,3,1,2,1,1,1,3,1,3,3,2,1,
	1,1,3,2,3,1,2,1,1,1,3,2,3,1,2,1,
	1,1,3,1,3,1,2,1,1,1,3,1,3,1,2,1,
	1,1,3,1,3,1,2,1,1,1,3,1,3,1,2,1
};

// returns the instruction length in bytes
auto dev::Debugger::GetCmdLen(const uint8_t _opcode) const -> const uint8_t
{
	return cmd_lens[_opcode];
}

// returns the mnemonic for an instruction
auto GetMnemonic(const uint8_t _opcode, const uint8_t _dataL, const uint8_t _dataH)
-> const std::string
{
	std::string out(mnemonics[_opcode]);

	if (cmd_lens[_opcode] == 2)
	{
		out += std::format(" 0x{:02X}", _dataL);
	}
	else if (cmd_lens[_opcode] == 3)
	{
		auto dataW = _dataH << 8 | _dataL;
		out += std::format(" 0x{:04X}", dataW);
	}
	return out;
}

// 0 - call
// 1 - c*
// 2 - rst
// 3 - pchl
// 4 - jmp, 
// 5 - j*
// 6 - ret, r*
// 7 - other
static const uint8_t opcode_types[0x100] =
{
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,

	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,

	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,

	6, 7, 5, 4, 1, 7, 7, 2, 6, 6, 5, 7, 1, 0, 7, 2,
	6, 7, 5, 7, 1, 7, 7, 2, 6, 7, 5, 7, 1, 7, 7, 2,
	6, 7, 5, 7, 1, 7, 7, 2, 6, 3, 5, 7, 1, 7, 7, 2,
	6, 7, 5, 7, 1, 7, 7, 2, 6, 7, 5, 7, 1, 7, 7, 2,
};

#define OPCODE_TYPE_MAX 7

// returns the type of an instruction
inline uint8_t get_opcode_type(const uint8_t _opcode)
{
	return opcode_types[_opcode];
}

#define OPCODE_PCHL 0xE9

// disassembles a data byte
auto dev::Debugger::GetDisasmLineDb(const uint8_t _data) const
->const std::string
{
	return std::format("DB 0x{:02X}", _data);
}

// disassembles an instruction
auto dev::Debugger::GetDisasmLine(const uint8_t _opcode, const uint8_t _dataL, const uint8_t _dataH) const
->const std::string
{
	return GetMnemonic(_opcode, _dataL, _dataH);
}

// shifts the addr by _instructionsOffset instruction counter
// if _instructionsOffset=3, it returns the addr of a third instruction after _addr, and vice versa
#define MAX_ATTEMPTS 41 // max attemts to find an addr of an instruction before _addr 
auto dev::Debugger::GetAddr(const Addr _addr, const int _instructionOffset) const
-> Addr
{
	int instructions = _instructionOffset > 0 ? _instructionOffset : -_instructionOffset;

	if (_instructionOffset > 0)
	{
		Addr addr = _addr;
		for (int i = 0; i < instructions; i++)
		{
			auto opcode = m_hardware.Request(Hardware::Req::GET_BYTE_RAM, { { "addr", addr } })->at("data");

			auto cmdLen = GetCmdLen(opcode);
			addr = addr + cmdLen;
		}
		return addr;
	}
	else if (_instructionOffset < 0)
	{
		std::vector<Addr> possibleDisasmStartAddrs;

		int disasmStartAddr = _addr - instructions * CMD_LEN_MAX;

		for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++)
		{
			int addr = disasmStartAddr;
			int currentInstruction = 0;

			while (addr < _addr && currentInstruction < instructions)
			{
				auto opcode = m_hardware.Request(Hardware::Req::GET_BYTE_RAM, { { "addr", addr } })->at("data");

				auto cmdLen = GetCmdLen(opcode);
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
		if (possibleDisasmStartAddrs.empty()) return _addr;

		// get the best result basing on the execution counter
		for (const auto& possibleDisasmStartAddr : possibleDisasmStartAddrs)
		{
			if (m_memRuns[possibleDisasmStartAddr] > 0) return possibleDisasmStartAddr;
		}
		return possibleDisasmStartAddrs[0];
	}

	return _addr;
}

// _instructionsOffset defines the start address of the disasm. 
// -5 means the start address is 5 instructions prior the _addr, and vise versa.
auto dev::Debugger::GetDisasm(const Addr _addr, const size_t _lines, const int _instructionOffset)
->Disasm
{
	Disasm out;
	if (_lines == 0) return out;

	auto instructionsOffset = _instructionOffset > 0 ? _instructionOffset : -_instructionOffset;

	int lines = (int)_lines;

	// calculate a new address that precedes the specified 'addr' by the instructionsOffset
	Addr addr = GetAddr(_addr, _instructionOffset);

	if (_instructionOffset < 0 && addr == _addr)
	{
		// it failed to find an new addr, we assume a data blob is ahead
		addr -= (Addr)instructionsOffset;
		lines -= instructionsOffset;

		for (int i = 0; i < instructionsOffset; i++)
		{
			if (m_labels.contains(addr))
			{
				DisasmLine disasmLine(DisasmLine::Type::LABELS, addr, GetDisasmLabels(addr));
				out.emplace_back(std::move(disasmLine));
			}
			
			auto db = m_hardware.Request(Hardware::Req::GET_BYTE_RAM, { { "addr", addr } })->at("data");
			auto globalAddr = m_hardware.Request(Hardware::Req::GET_GLOBAL_ADDR_RAM, { { "addr", addr } })->at("data");

			auto breakpointStatus = GetBreakpointStatus(globalAddr);
			DisasmLine lineS(DisasmLine::Type::CODE, addr, GetDisasmLineDb(db), m_memRuns[globalAddr], m_memReads[globalAddr], m_memWrites[globalAddr], "", breakpointStatus);
			out.emplace_back(lineS);

			addr++;
		}
	}

	for (int i = 0; i < lines; i++)
	{
		auto opcode = m_hardware.Request(Hardware::Req::GET_BYTE_RAM, { { "addr", addr } })->at("data");
		auto dataL = m_hardware.Request(Hardware::Req::GET_BYTE_RAM, { { "addr", addr + 1 } })->at("data");
		auto dataH = m_hardware.Request(Hardware::Req::GET_BYTE_RAM, { { "addr", addr + 2 } })->at("data");

		if (m_labels.contains(addr))
		{
			DisasmLine disasmLine(DisasmLine::Type::LABELS, addr, GetDisasmLabels(addr));
			out.emplace_back(std::move(disasmLine));
		}

		std::string consts;
		if (GetCmdLen(opcode) == 2)
		{
			consts = LabelsToStr(dataL, LABEL_TYPE_CONST);
		}
		else if (GetCmdLen(opcode) == 3)
		{
			consts = LabelsToStr(dataH << 8 | dataL, LABEL_TYPE_ALL);
		}

		auto globalAddr = m_hardware.Request(Hardware::Req::GET_GLOBAL_ADDR_RAM, { { "addr", addr } })->at("data");

		auto disasmS = GetDisasmLine(opcode, dataL, dataH);
		auto breakpointStatus = GetBreakpointStatus(globalAddr);
		DisasmLine lineS(DisasmLine::Type::CODE, addr, disasmS, m_memRuns[globalAddr], m_memReads[globalAddr], m_memWrites[globalAddr], consts, breakpointStatus);
		out.emplace_back(lineS);

		addr = addr + GetCmdLen(opcode);
	}
	return out;
}

bool IsConstLabel(const char* _s)
{
	// Iterate through each character in the string
	while (*_s != '\0') {
		// Check if the character is uppercase or underscore
		if (!(std::isupper(*_s) || *_s == '_' || (*_s >= '0' && *_s <= '9'))) {
			return false; // Not all characters are capital letters or underscores
		}
		++_s; // Move to the next character
	}
	return true; // All characters are capital letters or underscores
}

void dev::Debugger::LoadLabels(const std::wstring& _path)
{
	ResetLabels();

	if (!dev::IsFileExist(_path)) return;

	// load labels
	auto lines = dev::LoadTextFile(_path);
	if (lines.empty()) return;
	
	for( const auto& line : lines)
	{
		//char* labelBeforePeriod_c = strtok_s(label_c, " .&\n", &labelContext);
		auto label_addr = dev::Split(line, ' ');
		if (label_addr.size() != 2) continue;

		const auto& labelName = label_addr[0];
		auto addr = StrHexToInt(label_addr[1].c_str()+1); // +1 to skip the '$' char
		// skip if the label name is empty or the addr is not valid
		if (labelName.empty() ||  addr < 0 || addr > Memory::GLOBAL_MEMORY_LEN) continue;
		// skip if the label name contains '.' meaning it is a labl in the Macro. Macros are not supported in the disasm, so skip it.
		if (labelName.find('.') != std::string::npos) continue;

		if (IsConstLabel(labelName.c_str()))
		{
			m_consts.emplace(addr, AddrLabels{}).first->second.emplace_back(labelName);
		}
		// check if it is an __external_label
		else if (labelName.size() >= 2 && labelName[0] == '_' && labelName[1] == '_')
		{
			m_externalLabels.emplace(addr, AddrLabels{}).first->second.emplace_back(labelName);
		}
		else
		{
			m_labels.emplace(addr, AddrLabels{}).first->second.emplace_back(labelName);
		}
	}
}

void dev::Debugger::ResetLabels()
{
	m_labels.clear();
	m_consts.clear();
	m_externalLabels.clear();
}

//////////////////////////////////////////////////////////////
//
// Tracelog
//
//////////////////////////////////////////////////////////////

// a hardware thread
void dev::Debugger::TraceLogUpdate(const GlobalAddr _globalAddr, const uint8_t _opcode, const uint8_t _dataH, const uint8_t _dataL, const Addr _hl)
{
	m_traceLogIdx = (m_traceLogIdx - 1) % TRACE_LOG_SIZE;
	m_traceLog[m_traceLogIdx].m_globalAddr = _globalAddr;
	m_traceLog[m_traceLogIdx].m_opcode = _opcode;

	if (_opcode == OPCODE_PCHL)
	{
		m_traceLog[m_traceLogIdx].m_dataL = _hl & 0xff;
		m_traceLog[m_traceLogIdx].m_dataH = _hl >> 8;
	}
	else {
		m_traceLog[m_traceLogIdx].m_dataL = _dataL;
		m_traceLog[m_traceLogIdx].m_dataH = _dataH;
	}
}

auto dev::Debugger::GetTraceLog(const int _offset, const size_t _lines, const size_t _filter)
->std::string
{
	size_t filter = _filter > OPCODE_TYPE_MAX ? OPCODE_TYPE_MAX : _filter;
	size_t offset = _offset < 0 ? -_offset : _offset;

	std::string out;

	for (int i = 0; i < offset; i++)
	{
		m_traceLogIdxViewOffset = TraceLogNextLine(m_traceLogIdxViewOffset, _offset < 0, filter);
	}

	size_t idx = m_traceLogIdx + m_traceLogIdxViewOffset;
	size_t idx_last = m_traceLogIdx + TRACE_LOG_SIZE - 1;
	size_t line = 0;

	size_t first_line_idx = TraceLogNearestForwardLine(m_traceLogIdx, filter);

	for (; idx <= idx_last && line < _lines; idx++)
	{
		if (m_traceLog[idx % TRACE_LOG_SIZE].m_globalAddr < 0) break;

		if (get_opcode_type(m_traceLog[idx % TRACE_LOG_SIZE].m_opcode) <= filter)
		{
			std::string lineS;

			if (idx == first_line_idx)
			{
				lineS += ">";
			}
			else
			{
				lineS += " ";
			}
			lineS += m_traceLog[idx % TRACE_LOG_SIZE].ToStr();

			const Addr operand_addr = m_traceLog[idx % TRACE_LOG_SIZE].m_dataH << 8 | m_traceLog[idx % TRACE_LOG_SIZE].m_dataL;

			lineS += LabelsToStr(operand_addr, LABEL_TYPE_ALL);
			if (line != 0)
			{
				lineS += "\n";
			}

			out = lineS + out;

			line++;
		}
	}

	return out;
}

auto dev::Debugger::TraceLogNextLine(const int _idxOffset, const bool _reverse, const size_t _filter) const
->int
{
	size_t filter = _filter > OPCODE_TYPE_MAX ? OPCODE_TYPE_MAX : _filter;

	size_t idx = m_traceLogIdx + _idxOffset;
	size_t idx_last = m_traceLogIdx + TRACE_LOG_SIZE - 1;

	int dir = _reverse ? -1 : 1;
	// for a forward scrolling we need to go to the second line if we were not exactly at the filtered line
	bool forward_second_search = false;
	size_t first_line_idx = idx;

	for (; idx >= m_traceLogIdx && idx <= idx_last; idx += dir)
	{
		if (get_opcode_type(m_traceLog[idx % TRACE_LOG_SIZE].m_opcode) <= filter)
		{
			if ((!_reverse && !forward_second_search) ||
				(_reverse && idx == first_line_idx && !forward_second_search))
			{
				forward_second_search = true;
				continue;
			}
			else
			{
				return (int)(idx - m_traceLogIdx);
			}
		}
	}

	return _idxOffset; // fails to reach the next line
}

auto dev::Debugger::TraceLogNearestForwardLine(const size_t _idx, const size_t _filter) const
->int
{
	size_t filter = _filter > OPCODE_TYPE_MAX ? OPCODE_TYPE_MAX : _filter;

	size_t idx = _idx;
	size_t idx_last = m_traceLogIdx + TRACE_LOG_SIZE - 1;

	for (; idx >= m_traceLogIdx && idx <= idx_last; idx++)
	{
		if (get_opcode_type(m_traceLog[idx % TRACE_LOG_SIZE].m_opcode) <= filter)
		{
			return (int)idx;
		}
	}

	return (int)_idx; // fails to reach the nearest line
}

auto dev::Debugger::TraceLog::ToStr() const
->std::string
{
	return std::format("0x{:05X}: {}", m_globalAddr, GetMnemonic(m_opcode, m_dataL, m_dataH));
}

void dev::Debugger::TraceLog::Clear()
{
	m_globalAddr = -1;
	m_opcode = 0;
	m_dataL = 0;
	m_dataH = 0;
}

//////////////////////////////////////////////////////////////
//
// Debug flow
//
//////////////////////////////////////////////////////////////

// an external thread
bool dev::Debugger::CheckBreak(GlobalAddr _globalAddr)
{
	if (m_wpBreak)
	{
		m_wpBreak = false;
		//PrintWatchpoints();
		//ResetWatchpoints();
		return true;
	}

	auto break_ = CheckBreakpoints(_globalAddr);

	//if (break_) PrintWatchpoints();

	return break_;
}

//////////////////////////////////////////////////////////////
//
// Breakpoints
//
//////////////////////////////////////////////////////////////

void dev::Debugger::SetBreakpointStatus(const GlobalAddr _globalAddr, const Breakpoint::Status _status)
{
	if (_status == Breakpoint::Status::DELETED)
	{
		DelBreakpoint(_globalAddr);
		return;
	}
	auto bpI = m_breakpoints.find(_globalAddr);
	if (bpI != m_breakpoints.end()) {
		bpI->second.SetStatus(_status);
		return;
	}
	AddBreakpoint(_globalAddr, _status);
}

void dev::Debugger::AddBreakpoint(const GlobalAddr _globalAddr, 
	const Breakpoint::Status _status, const std::string& _comment)
{
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	auto bpI = m_breakpoints.find(_globalAddr);
	if (bpI != m_breakpoints.end())
	{
		m_breakpoints.erase(bpI);
	}

	m_breakpoints.emplace(_globalAddr, std::move(Breakpoint(_globalAddr, _status, _comment)));
}

void dev::Debugger::DelBreakpoint(const GlobalAddr _globalAddr)
{
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	auto bpI = m_breakpoints.find(_globalAddr);
	if (bpI != m_breakpoints.end())
	{
		m_breakpoints.erase(bpI);
	}
}

bool dev::Debugger::CheckBreakpoints(const GlobalAddr _globalAddr)
{
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	auto bpI = m_breakpoints.find(_globalAddr);
	if (bpI == m_breakpoints.end()) return false;
	return bpI->second.CheckStatus();
}
/*
void dev::Debugger::PrintBreakpoints()
{
	std::printf("breakpoints:\n");
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	for (const auto& [addr, bp] : m_breakpoints)
	{
		bp.Print();
	}
}
*/
auto dev::Debugger::GetBreakpoints() -> const Breakpoints
{
	Breakpoints out;
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	for (const auto& [addr, bp] : m_breakpoints)
	{
		out.insert({ addr, bp });
	}
	return out;
}

auto dev::Debugger::GetBreakpointStatus(const GlobalAddr _globalAddr)
-> const Breakpoint::Status
{
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	auto bpI = m_breakpoints.find(_globalAddr);

	return bpI == m_breakpoints.end() ? Breakpoint::Status::DELETED : bpI->second.GetStatus();
}

//////////////////////////////////////////////////////////////
//
// Watchpoint
//
//////////////////////////////////////////////////////////////

void dev::Debugger::AddWatchpoint(
	const Watchpoint::Id _id, const Watchpoint::Access _access, 
	const GlobalAddr _globalAddr, const Watchpoint::Condition _cond,
	const uint16_t _value, const size_t _value_size, const bool _active, const std::string& _comment)
{
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
	
	auto wpI = m_watchpoints.find(_id);
	if (wpI != m_watchpoints.end())
	{
		wpI->second.Update(_access, _globalAddr, _cond, _value, _value_size, _active, _comment);
	}
	else {
		const auto wp = Watchpoint(_access, _globalAddr, _cond, _value, _value_size, _active, _comment);
		m_watchpoints.emplace(wp.GetId(), std::move(wp));
	}
}

void dev::Debugger::DelWatchpoint(const Watchpoint::Id _id)
{
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);

	auto bpI = m_watchpoints.find(_id);
	if (bpI != m_watchpoints.end())
	{
		m_watchpoints.erase(bpI);
	}
}

// a hardware thread
bool dev::Debugger::CheckWatchpoint(const Watchpoint::Access _access, const GlobalAddr _globalAddr, const uint8_t _value)
{
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
	auto wpI = WatchpointsFind(_globalAddr);
	if (wpI == m_watchpoints.end()) return false;

	auto out = wpI->second.Check(_access, _globalAddr, _value);
	/*
	if (out) {
		auto data = m_memory.GetWord(_globalAddr, Memory::AddrSpace::RAM);
		std::printf("wp break = true. addr: 0x%05x, word: 0x%02x \n", _globalAddr, data);
	}
	*/
	return out;
}

void dev::Debugger::ResetWatchpoints()
{
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
	for (auto& [id, watchpoint] : m_watchpoints)
	{
		watchpoint.Reset();
	}
}
/*
void dev::Debugger::PrintWatchpoints()
{
	std::printf("watchpoints:\n");
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
	for (const auto& wp : m_watchpoints)
	{
		wp.Print();
	}
}
*/

auto dev::Debugger::WatchpointsFind(const GlobalAddr _globalAddr)
->Watchpoints::iterator
{
	return std::find_if(m_watchpoints.begin(), m_watchpoints.end(), 
		[_globalAddr](const Watchpoints::value_type& pair) 
		{
			return pair.second.CheckAddr(_globalAddr);
		});
}

auto dev::Debugger::GetWatchpoints() -> const Watchpoints
{
	Watchpoints out;
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	for (const auto& [id, wp] : m_watchpoints)
	{
		out.emplace(id, Watchpoint{ wp });
	}
	return out;
}

//////////////////////////////////////////////////////////////
//
// Utils
//
//////////////////////////////////////////////////////////////

auto dev::Debugger::GetDisasmLabels(const Addr _addr) const
-> const std::string
{
	std::string out;

	if (m_labels.contains(_addr))
	{
		int i = 0;
		for (const auto& label : m_labels.at(_addr))
		{
			out += label;
			if (i == 0)
			{
				out += ":\t";
			}
			else
			{
				out += ", ";
			}
			i++;
		}
	}
	return out;
}

auto dev::Debugger::LabelsToStr(const Addr _addr, int _labelTypes) const
-> const std::string
{
	std::string out;

	if (_labelTypes & LABEL_TYPE_LABEL && m_labels.contains(_addr))
	{
		for (const auto& label : m_labels.at(_addr))
		{
			out += label + ", ";
		}
	}
	if (_labelTypes & LABEL_TYPE_CONST && m_consts.contains(_addr))
	{
		for (const auto& label : m_consts.at(_addr))
		{
			out += label + ", ";
		}
	}
	if (_labelTypes & LABEL_TYPE_EXTERNAL && m_externalLabels.contains(_addr))
	{
		for (const auto& label : m_externalLabels.at(_addr))
		{
			out += label + ", ";
		}
	}

	return out;
}

//////////////////////////////////////////////////////////////
//
// Requests
//
//////////////////////////////////////////////////////////////

void dev::Debugger::ReqLoadRom(const std::wstring& _path)
{
	m_pathLast = _path;

	auto fileSize = GetFileSize(_path);

	if (fileSize > Memory::MEMORY_MAIN_LEN) {
		// TODO: communicate the fail state
		return;
	}

	auto result = dev::LoadFile(_path);
	
	if (!result || result->empty()) {
		// TODO: communicate the fail state
		return;
	}

	m_hardware.Request(Hardware::Req::SET_MEM, *result);

	Log("file loaded: {}", dev::StrWToStr(_path));
}

void dev::Debugger::ReqLoadRomLast()
{
	if (m_pathLast.empty()) return;

	ReqLoadRom(m_pathLast);
}