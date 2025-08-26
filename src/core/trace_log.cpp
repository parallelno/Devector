#include "core/trace_log.h"

#include <chrono>
#include <format>

#include "utils/str_utils.h"

dev::TraceLog::TraceLog(const DebugData& _debugData)
	:
	m_debugData(_debugData)
{}

// Hardware thread
void dev::TraceLog::Update(
	const CpuI8080::State& _cpuState, const Memory::State& _memState)
{
	UpdateLogBuffer(_cpuState, _memState);

	SaveLog(_cpuState, _memState);
}

void dev::TraceLog::UpdateLogBuffer(
	const CpuI8080::State& _cpuState, const Memory::State& _memState)
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
	m_log[m_logIdx].imm.l = opcode != CpuI8080::OPCODE_PCHL ?
		dataL :
		_cpuState.regs.hl.l;
	m_log[m_logIdx].imm.h = opcode != CpuI8080::OPCODE_PCHL ?
		dataH :
		_cpuState.regs.hl.h;
}

// UI thread
auto dev::TraceLog::GetDisasm(const size_t _lines, const uint8_t _filter)
-> const Lines*
{
	size_t idxLast = m_logIdx + TRACE_LOG_SIZE - 1;
	m_disasmLinesLen = 0;
	auto idx = m_logIdx;
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
	auto immType = GetImmediateType(_item.opcode);

	_line.Init();

	_line.opcode = _item.opcode;
	switch (GetCmdLen(_item.opcode))
	{
	case 1:
		_line.imm = immType == CMD_IB_OFF0 ? _item.opcode : 0;
		break;
	case 2:
		_line.imm = _item.imm.l;
		break;
	case 3:
		_line.imm = _item.imm.word;
		break;
	};

	if (immType != CMD_IM_NONE)
	{
		auto labelsP = m_debugData.GetLabels(_line.imm);
		_line.labels = labelsP ? std::move(*labelsP) : Disasm::LabelList();
		auto constsP = m_debugData.GetConsts(_line.imm);
		_line.consts = constsP ? *constsP : Disasm::LabelList();
	}

	_line.type = Disasm::Line::Type::CODE;
	_line.addr = (Addr)_item.globalAddr;
}


void dev::TraceLog::Reset()
{
	m_disasmLinesLen = m_logIdx = 0;
	m_log[m_logIdx].globalAddr = EMPTY_ITEM;

	// create a new log file
	if (m_saveLogInited)
	{
		SetSaveLog(false);
		SetSaveLog(true);
	}
}


void dev::TraceLog::SetSaveLog(bool _saveLog)
{
	if (_saveLog)
	{
		if (!m_saveLogInited)
		{
			m_saveLogPath = GetLogPath();

			// create the log file
			m_logFile.open(m_saveLogPath, std::ios::out | std::ios::trunc);

			// handle error
			if (!m_logFile)
			{
				dev::Log(
					"Trace Log: Failed to open log file: {}", m_saveLogPath);
				m_saveLog = false;
			}
			else{
				m_saveLogInited = true;
				m_saveLog = true;
			}
		}
	}
	else{
		if (m_logFile.is_open()){
			m_logFile.close();
		}
		m_saveLogInited = false;
		m_saveLog = false;
	}
}


void dev::TraceLog::SaveLog(
	const CpuI8080::State& _cpuState, const Memory::State& _memState)
{
	if (m_saveLog && m_logFile.is_open())
	{
		m_logFile << dev::GetDisasmLogLine(_cpuState, _memState);
	}
}


auto dev::TraceLog::GetLogPath()
-> std::string
{
	auto executableDir = dev::GetExecutableDir();

	auto now = std::chrono::system_clock::now();
    auto local_time = std::chrono::current_zone()->to_local(now);

	auto saveLogFilename = std::format("{}_{:%Y-%m-%d_%H-%M}.txt",
		TRACE_LOG_NAME, local_time);

	return executableDir + saveLogFilename;
}
