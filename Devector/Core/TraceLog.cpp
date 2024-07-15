#include "Core/TraceLog.h"


dev::TraceLog::TraceLog(const DebugData& _debugData)
	: 
	m_debugData(_debugData)
{}

// a hardware thread
void dev::TraceLog::Update(const CpuI8080::State& _cpuState, const Memory::State& _memState)
{
	uint8_t opcode = _memState.debug.instr[0];
	uint8_t dataL = _memState.debug.instr[1];
	uint8_t dataH = _memState.debug.instr[2];

	// skip repeataive HLT
	if (opcode == CpuI8080::OPCODE_HLT &&
		m_log[m_logIdx].opcode == CpuI8080::OPCODE_HLT) {
		return;
	}

	m_logIdx = --m_logIdx % TRACE_LOG_SIZE;
	m_log[m_logIdx].globalAddr = _memState.debug.instrGlobalAddr;
	m_log[m_logIdx].opcode = opcode;
	m_log[m_logIdx].imm.l = opcode != CpuI8080::OPCODE_PCHL ? dataL : _cpuState.regs.hl.l;
	m_log[m_logIdx].imm.h = opcode != CpuI8080::OPCODE_PCHL ? dataH : _cpuState.regs.hl.h;
}

auto dev::TraceLog::GetDisasm(const size_t _lines, const uint8_t _filter)
-> const Lines*
{
	size_t idxLast = m_logIdx + TRACE_LOG_SIZE - 1;
	m_disasmLinesLen = 0;
	int idx = m_logIdx;
	int line = 0;

	for (; idx <= idxLast && line < _lines; idx++)
	{
		auto& item = m_log[idx % TRACE_LOG_SIZE];
		auto globalAddr = item.globalAddr;

		if (globalAddr == EMPTY_ITEM) { break; }

		if (GetOpcodeType(item.opcode) <= _filter)
		{
			AddCode(item, m_disasmLines[m_disasmLinesLen++]);
			line++;
		}
	}

	return &m_disasmLines;
}

void dev::TraceLog::AddCode(const Item& _item, Disasm::Line& _line)
{
	GlobalAddr globalAddr = _item.globalAddr;
	uint8_t opcode = _item.opcode;
	auto immType = GetImmediateType(opcode);

	_line.Init();

	if (immType == CMD_IB_OFF0) {
		_line.opcode = 0x10;
	}
	else {
		_line.opcode = opcode;
	}

	auto cmdLen = GetCmdLen(opcode);
	uint16_t imm = 0;
	switch (cmdLen)
	{
	case 1:
		imm = 0;
		break;
	case 2:
		imm = _item.imm.l;
		break;
	case 3:
		imm = _item.imm.word;
		break;
	};

	if (immType != CMD_IM_NONE) 
	{
		/*_line.labels = m_debugData.GetLabels(imm);
		_line.consts = m_debugData.GetConsts(imm);*/
	}
	{
		_line.labels = nullptr;
		_line.consts = nullptr;
		_line.comment = nullptr;
	}

	_line.type = Disasm::Line::Type::CODE;
	_line.addr = (Addr)globalAddr;
	_line.imm = imm;
}

void dev::TraceLog::Reset()
{
	m_disasmLinesLen = m_logIdx = 0;
	m_log[m_logIdx].globalAddr = EMPTY_ITEM;
}

