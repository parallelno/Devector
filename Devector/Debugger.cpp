#include "Debugger.h"
#include <format>
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>
#include "Utils/StringUtils.h"
#include "Utils/Utils.h"

dev::Debugger::Debugger(I8080& _cpu, Memory& _memory)
    : 
	m_cpu(_cpu),
	m_memory(_memory),
    m_wpBreak(false),
	m_traceLog()
{
    Init();
}

void dev::Debugger::Init()
{
    std::fill(m_memRuns, m_memRuns + std::size(m_memRuns), 0);
    std::fill(m_memReads, m_memReads + std::size(m_memReads), 0);
    std::fill(m_memWrites, m_memWrites + std::size(m_memWrites), 0);

	for (size_t i = 0; i < TRACE_LOG_SIZE; i++)
	{
		m_traceLog[i].Clear();
	}
	m_traceLogIdx = 0;
	m_traceLogIdxViewOffset = 0;

	if (!m_cpu.DebugOnRead) {
		m_cpu.DebugOnRead = std::bind(&Debugger::Read, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	}
	if (!m_cpu.DebugOnWrite) {
		m_cpu.DebugOnWrite = std::bind(&Debugger::Write, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	}
}

void dev::Debugger::Read(
    const uint32_t _addr, Memory::AddrSpace _addrSpace, const uint8_t _val, const bool _isOpcode)
{
    auto globalAddr = m_memory.GetGlobalAddr(_addr, _addrSpace);

    if (_isOpcode) {
        m_memRuns[globalAddr]++;
        TraceLogUpdate(globalAddr, _val);
    }
    else {
        m_memReads[globalAddr]++;
        m_wpBreak |= CheckWatchpoint(Watchpoint::Access::R, globalAddr, _val);
    }
}

void dev::Debugger::Write(const uint32_t _addr, Memory::AddrSpace _addrSpace, const uint8_t _val)
{
    auto globalAddr = m_memory.GetGlobalAddr(_addr, _addrSpace);
    m_memWrites[globalAddr]++;
    m_wpBreak |= CheckWatchpoint(Watchpoint::Access::W, globalAddr, _val);
}


//////////////////////////////////////////////////////////////
//
// Disasm
//
//////////////////////////////////////////////////////////////

static const char* mnemonics[0x100] =
{
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
};

// define the maximum number of bytes in a command
#define CMD_BYTES_MAX 3

// array containing lengths of commands, indexed by opcode
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
auto dev::Debugger::GetCmdLen(const uint8_t _addr) const -> const size_t
{
	return cmd_lens[_addr];
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
auto dev::Debugger::GetDisasmLineDb(const uint32_t _addr, const uint8_t _data) const
->const std::string
{
	return std::format("0x{:04X}\tDB 0x{:02X}\t", _addr, _data);
}

// disassembles an instruction
auto dev::Debugger::GetDisasmLine(const uint32_t _addr, const uint8_t _opcode, const uint8_t _dataL, const uint8_t _dataH) const
->const std::string
{
	return std::format("0x{:04X}\t{}\t", _addr, GetMnemonic(_opcode, _dataL, _dataH));
}

// calculates the start address for a range of instructions before a given address
size_t dev::Debugger::GetAddr(const uint32_t _endAddr, const size_t _beforeAddrLines) const
{
	if (_beforeAddrLines == 0) return _endAddr;

	size_t start_addr = (_endAddr - _beforeAddrLines * CMD_BYTES_MAX) & 0xffff;
#define MAX_ATTEMPTS 41

	int lines = 0;
	int addr_diff_max = (int)_beforeAddrLines * CMD_BYTES_MAX + 1;
	size_t addr = start_addr & 0xffff;

	for (int attempt = MAX_ATTEMPTS; attempt > 0 && lines != _beforeAddrLines; attempt--)
	{
		addr = start_addr & 0xffff;
		int addr_diff = addr_diff_max;
		lines = 0;

		while (addr_diff > 0 && addr != _endAddr)
		{
			auto opcode = m_memory.GetByte(addr, Memory::AddrSpace::RAM);
			auto cmd_len = GetCmdLen(opcode);
			addr = (addr + cmd_len) & 0xffff;
			addr_diff -= cmd_len;
			lines++;
		}

		if (addr == _endAddr && lines == _beforeAddrLines)
		{
			return start_addr;
		}

		start_addr++;
		addr_diff_max--;
	}
	return _endAddr;
}

auto dev::Debugger::GetDisasm(const uint32_t _addr, const size_t _lines, const size_t _beforeAddrLines) const
->std::vector<std::string>
{
	std::vector<std::string> out;
	if (_lines == 0) return out;


	uint32_t addr = _addr;
	auto pc = m_cpu.m_pc;
	int lines = _lines;

	if (_beforeAddrLines > 0)
	{
		// calculate a new address that precedes the specified 'addr' by the before_addr_lines number of command lines.
		addr = GetAddr(_addr & 0xffff, _beforeAddrLines) & 0xffff;

		// If it fails to find an new addr, we assume a data blob is ahead
		if (addr == _addr)
		{
			addr = (addr - _beforeAddrLines) & 0xffff;

			lines = _lines - _beforeAddrLines;

			for (int i = 0; i < _beforeAddrLines; i++)
			{
				std::string lineS;

				auto db = m_memory.GetByte(addr, Memory::AddrSpace::RAM);
				lineS += GetDisasmLineDb(addr, db);

				size_t globalAddr = m_memory.GetGlobalAddr(addr, Memory::AddrSpace::RAM);
				std::string runsS = std::to_string(m_memRuns[globalAddr]);
				std::string readsS = std::to_string(m_memReads[globalAddr]);
				std::string writesS = std::to_string(m_memWrites[globalAddr]);
				std::string runsReadsWritesS = runsS + "," + readsS + "," + writesS + "\t";
				lineS += runsReadsWritesS;

				if (m_labels.contains(addr & 0xffff))
				{
					out.push_back(GetDisasmLabels(addr & 0xffff));
				}

				out.push_back(lineS);

				addr = (addr + 1) & 0xffff;
			}
		}
	}

	for (int i = 0; i < lines; i++)
	{
		std::string lineS;

		auto opcode = m_memory.GetByte(addr, Memory::AddrSpace::RAM);
		auto dataL = m_memory.GetByte(addr + 1, Memory::AddrSpace::RAM);
		auto dataH = m_memory.GetByte(addr + 2, Memory::AddrSpace::RAM);
		lineS += GetDisasmLine(addr, opcode, dataL, dataH);

		size_t globalAddr = m_memory.GetGlobalAddr(addr, Memory::AddrSpace::RAM);
		std::string runsS = std::to_string(m_memRuns[globalAddr]);
		std::string readsS = std::to_string(m_memReads[globalAddr]);
		std::string writesS = std::to_string(m_memWrites[globalAddr]);
		std::string runsReadsWritesS = runsS + "," + readsS + "," + writesS + "\t";
		lineS += runsReadsWritesS;

		if (m_labels.contains(addr & 0xffff))
		{
			out.push_back(GetDisasmLabels((uint16_t)addr));
		}

		if (GetCmdLen(opcode) == 3 || opcode == OPCODE_PCHL)
		{
			int operand_addr = 0;
			if (opcode == OPCODE_PCHL)
			{
				operand_addr = m_cpu.GetHL();
			}
			else
			{
				operand_addr = dataH << 8 | dataL;
			}

			lineS += LabelsToStr(operand_addr & 0xffff, LABEL_TYPE_ALL);
		}

		out.push_back(lineS);

		addr = (addr + GetCmdLen(opcode)) & 0xffff;
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
	// load labels
	auto result = dev::LoadFile(_path);
	if (!result) return;

	char* labels_c(reinterpret_cast<char*>(result->data()));
	
	char* labelValPairContext = nullptr; // for strtok_s
	
	char* label_c = strtok_s(labels_c, " $\n", &labelValPairContext);
	char* addr_c = strtok_s(nullptr, " $\n", &labelValPairContext);

	m_labels.clear();
	m_consts.clear();
	m_externalLabels.clear();
	
	int addr = 0;
	char* end;

	char* labelContext = nullptr; // for strtok_s

	while (label_c != nullptr)
	{
		if (label_c == nullptr || addr_c == nullptr) break;

		char* labelBeforePeriod_c = strtok_s(label_c, " .&\n", &labelContext);

		addr = strtol(addr_c, &end, 16);


		if (std::string(label_c) == "ram_disk_mode")
		{
			int temp = 1;
		}

		// check if it is a CONSTANT_NAME
		if (IsConstLabel(labelBeforePeriod_c))
		{
			if (!m_consts.contains(addr))
			{
				m_consts.emplace(addr, AddrLabels{ std::string(label_c) });
			}
			else
			{
				m_consts[addr].push_back(label_c);
			}
		}
		// check if it is an __external_label
		else if (strlen(label_c) >= 2 && label_c[0] == '_' && label_c[1] == '_')
		{
			if (!m_externalLabels.contains(addr))
			{
				m_externalLabels.emplace(addr, AddrLabels{ std::string(label_c) });
			}
			else
			{
				m_externalLabels[addr].push_back(label_c);
			}
		}
		else
		{
			if (!m_labels.contains(addr))
			{
				m_labels.emplace(addr, AddrLabels{ std::string(label_c) });
			}
			else
			{
				m_labels[addr].push_back(label_c);
			}
		}
		label_c = strtok_s(nullptr, " $\n", &labelValPairContext);
		addr_c = strtok_s(nullptr, " $\n", &labelValPairContext);
	}
}

//////////////////////////////////////////////////////////////
//
// Tracelog
//
//////////////////////////////////////////////////////////////

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

			const size_t operand_addr = m_traceLog[idx % TRACE_LOG_SIZE].m_dataH << 8 | m_traceLog[idx % TRACE_LOG_SIZE].m_dataL;

			lineS += LabelsToStr(operand_addr & 0xffff, LABEL_TYPE_ALL);
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

void dev::Debugger::TraceLogUpdate(const uint32_t _globalAddr, const uint8_t _val)
{
	auto last_global_addr = m_traceLog[m_traceLogIdx].m_globalAddr;
	auto last_opcode = m_traceLog[m_traceLogIdx].m_opcode;

	m_traceLogIdx = (m_traceLogIdx - 1) % TRACE_LOG_SIZE;
	m_traceLog[m_traceLogIdx].m_globalAddr = _globalAddr;
	m_traceLog[m_traceLogIdx].m_opcode = _val;
	m_traceLog[m_traceLogIdx].m_dataL = m_memory.GetByte(_globalAddr + 1, Memory::AddrSpace::RAM);
	m_traceLog[m_traceLogIdx].m_dataH = m_memory.GetByte(_globalAddr + 2, Memory::AddrSpace::RAM);

	if (_val == OPCODE_PCHL)
	{
		uint16_t pc = m_cpu.GetHL();
		m_traceLog[m_traceLogIdx].m_dataL = pc & 0xff;
		m_traceLog[m_traceLogIdx].m_dataH = pc >> 8 & 0xff;
	}
	else
	{
		m_traceLog[m_traceLogIdx].m_dataL = m_memory.GetByte(_globalAddr + 1, Memory::AddrSpace::RAM);
		m_traceLog[m_traceLogIdx].m_dataH = m_memory.GetByte(_globalAddr + 2, Memory::AddrSpace::RAM);
	}
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
				return idx - m_traceLogIdx;
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
			return idx;
		}
	}

	return _idx; // fails to reach the nearest line
}

auto dev::Debugger::TraceLog::ToStr() const
->std::string
{
	std::stringstream out;

	out << "0x";
	out << std::setw(5) << std::setfill('0');
	out << std::uppercase << std::hex << static_cast<int>(m_globalAddr) << ": ";

	out << GetMnemonic(m_opcode, m_dataL, m_dataH);

	return out.str();
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

bool dev::Debugger::CheckBreak()
{
	if (m_wpBreak)
	{
		m_wpBreak = false;
		PrintWatchpoints();
		ResetWatchpoints();
		return true;
	}

	auto pc = m_cpu.m_pc;
	auto globalAddr = m_memory.GetGlobalAddr(pc, Memory::AddrSpace::RAM);

	auto break_ = CheckBreakpoints(globalAddr);

	if (break_) PrintWatchpoints();

	return break_;
}

//////////////////////////////////////////////////////////////
//
// Breakpoints
//
//////////////////////////////////////////////////////////////

void dev::Debugger::AddBreakpoint(const uint32_t _addr, const bool _active, const Memory::AddrSpace _addrSpace)
{
	auto globalAddr = m_memory.GetGlobalAddr(_addr, _addrSpace);

	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	auto bp = m_breakpoints.find(globalAddr);
	if (bp != m_breakpoints.end())
	{
		m_breakpoints.erase(bp);
	}

	m_breakpoints.emplace(globalAddr, std::move(Breakpoint(globalAddr, _active)));
}

void dev::Debugger::DelBreakpoint(const uint32_t _addr, const Memory::AddrSpace _addrSpace)
{
	auto globalAddr = m_memory.GetGlobalAddr(_addr, _addrSpace);

	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	auto bp = m_breakpoints.find(globalAddr);
	if (bp != m_breakpoints.end())
	{
		m_breakpoints.erase(bp);
	}
}

bool dev::Debugger::CheckBreakpoints(const uint32_t _globalAddr)
{
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	auto bp = m_breakpoints.find(_globalAddr);
	if (bp == m_breakpoints.end()) return false;
	return bp->second.Check();
}

void dev::Debugger::PrintBreakpoints()
{
	std::printf("breakpoints:\n");
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	for (const auto& [addr, bp] : m_breakpoints)
	{
		bp.Print();
	}
}

auto dev::Debugger::Breakpoint::IsActive() const
->const bool
{
	return m_active;
}
auto dev::Debugger::Breakpoint::Check() const
->const bool
{
	return m_active;
}

void dev::Debugger::Breakpoint::Print() const
{
	std::printf("0x%06x, active: %d \n", m_globalAddr, m_active);
}

//////////////////////////////////////////////////////////////
//
// Watchpoint
//
//////////////////////////////////////////////////////////////

void dev::Debugger::AddWatchpoint(
	const Watchpoint::Access _access, const uint32_t _addr, const Watchpoint::Condition _cond,
	const uint16_t _value, const size_t _value_size, const bool _active, const Memory::AddrSpace _addrSpace)
{
	auto globalAddr = m_memory.GetGlobalAddr(_addr, _addrSpace);

	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
	m_watchpoints.emplace_back(std::move(Watchpoint(_access, globalAddr, _cond, _value, _value_size, _active)));
}

void dev::Debugger::DelWatchpoint(const uint32_t _addr, const Memory::AddrSpace _addrSpace)
{
	auto globalAddr = m_memory.GetGlobalAddr(_addr, _addrSpace);

	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
	WatchpointsErase(globalAddr);
}

bool dev::Debugger::CheckWatchpoint(const Watchpoint::Access _access, const uint32_t _globalAddr, const uint8_t _value)
{
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
	auto wp = WatchpointsFind(_globalAddr);
	if (wp == m_watchpoints.end()) return false;

	auto out = wp->Check(_access, _globalAddr, _value);

	if (out) {
		auto data = m_memory.GetWord(_globalAddr, Memory::AddrSpace::RAM);
		std::printf("wp break = true. addr: 0x%05x, word: 0x%02x \n", _globalAddr, data);
	}
	return out;
}

void dev::Debugger::ResetWatchpoints()
{
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
	for (auto& watchpoint : m_watchpoints)
	{
		watchpoint.Reset();
	}
}

void dev::Debugger::PrintWatchpoints()
{
	std::printf("watchpoints:\n");
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
	for (const auto& wp : m_watchpoints)
	{
		wp.Print();
	}
}

auto dev::Debugger::WatchpointsFind(const uint32_t _globalAddr)
->Watchpoints::iterator
{
	return std::find_if(m_watchpoints.begin(), m_watchpoints.end(), [_globalAddr](Watchpoint& w) {return w.CheckAddr(_globalAddr);});
}

void dev::Debugger::WatchpointsErase(const uint32_t _globalAddr)
{
	m_watchpoints.erase(std::remove_if(m_watchpoints.begin(), m_watchpoints.end(), [_globalAddr](Watchpoint const& _w)
		{
			return _w.GetGlobalAddr() == _globalAddr;
		}), m_watchpoints.end());

}


auto dev::Debugger::Watchpoint::IsActive() const
->const bool
{
	return m_active;
}

auto dev::Debugger::Watchpoint::Check(const Watchpoint::Access _access, const uint32_t _globalAddr, const uint8_t _value)
->const bool
{
	if (!m_active) return false;
	if (m_access != Access::RW && m_access != _access) return false;

	if (m_breakL && (m_valueSize == VAL_BYTE_SIZE || m_breakH)) return true;

	bool* break_p;
	uint8_t value_byte;

	if (_globalAddr == m_globalAddr)
	{
		break_p = &m_breakL;
		value_byte = m_value & 0xff;
	}
	else
	{
		break_p = &m_breakH;
		value_byte = m_value >> 8 & 0xff;
	}

	switch (m_cond)
	{
	case Condition::ANY:
		*break_p = true;
		break;
	case Condition::EQU:
		*break_p = _value == value_byte;
		break;
	case Condition::LESS:
		*break_p = _value < value_byte;
		break;
	case Condition::GREATER:
		*break_p = _value > value_byte;
		break;
	case Condition::LESS_EQU:
		*break_p = _value <= value_byte;
		break;
	case Condition::GREATER_EQU:
		*break_p = _value >= value_byte;
		break;
	case Condition::NOT_EQU:
		*break_p = _value != value_byte;
		break;
	default:
		return false;
	};

	return m_breakL && (m_valueSize == VAL_BYTE_SIZE || m_breakH);
}

auto dev::Debugger::Watchpoint::GetGlobalAddr() const
-> const size_t
{
	return m_globalAddr;
}

auto dev::Debugger::Watchpoint::CheckAddr(const uint32_t _globalAddr) const
-> const bool
{
	return _globalAddr == m_globalAddr || (_globalAddr == m_globalAddr + 1 && m_valueSize == VAL_WORD_SIZE);
}

void dev::Debugger::Watchpoint::Reset()
{
	m_breakL = false;
	m_breakH = false;
}

void dev::Debugger::Watchpoint::Print() const
{
	std::printf("0x%05x, access: %s, cond: %s, value: 0x%04x, value_size: %d, active: %d \n", m_globalAddr, access_s[static_cast<size_t>(m_access)], conditions_s[static_cast<size_t>(m_cond)], m_value, m_valueSize, m_active);
}

//////////////////////////////////////////////////////////////
//
// Utils
//
//////////////////////////////////////////////////////////////

auto dev::Debugger::GetDisasmLabels(uint16_t _addr) const
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

auto dev::Debugger::LabelsToStr(uint16_t _addr, int _labelTypes) const
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