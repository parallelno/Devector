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
	uint8_t opcode = _memState.debug.instr.opcode;

	// skip repeataive HLT
	if (opcode == CpuI8080::OPCODE_HLT &&
		m_log[m_logIdx].instr.opcode == CpuI8080::OPCODE_HLT) {
		return;
	}

	m_logIdx = --m_logIdx % TRACE_LOG_SIZE;

	m_log[m_logIdx].globalAddr = _memState.debug.instrGlobalAddr;
	m_log[m_logIdx].instr.opcode = _memState.debug.instr.opcode;
	m_log[m_logIdx].instr.dataW = opcode == CpuI8080::OPCODE_PCHL ?
		_cpuState.regs.pc.word : _memState.debug.instr.dataW;
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

		if (CpuI8080::GetInstrType(item.instr.opcode) <= _filter)
		{
			m_disasmLines[m_disasmLinesLen].InitInstr(globalAddr, item.instr, m_debugData, false);
			m_disasmLinesLen++;
			line++;
		}
	}

	return &m_disasmLines;
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


void dev::TraceLog::SetSaveLog(bool _saveLog, const std::string& _path)
{
	if (_saveLog)
	{
		if (!m_saveLogInited)
		{
			m_saveLogPath = !_path.empty() ? _path :
				!m_saveLogPath.empty() ? m_saveLogPath :
				GetDefaultLogPath();

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

std::array<char, dev::DisasmLine::LINE_BUFF_LEN> _traceLogBuffer = {};

void dev::TraceLog::SaveLog(
	const CpuI8080::State& _cpuState, const Memory::State& _memState)
{
	if (m_saveLog && m_logFile.is_open())
	{
		m_logFile << DisasmLine::PrintToBuffer(_traceLogBuffer,
											   _memState.debug.instrGlobalAddr,
											   _memState.debug.instr,
											   &_cpuState.regs );
	}
}


auto dev::TraceLog::GetDefaultLogPath()
-> std::string
{
	return dev::GetExecutableDir() + GetLogFilename();
}

auto dev::TraceLog::GetLogFilename()
-> std::string
{
	auto now = std::chrono::system_clock::now();
	auto local_time = std::chrono::current_zone()->to_local(now);

	auto saveLogFilename = std::format("{}_{:%Y-%m-%d_%H-%M}.txt",
		TRACE_LOG_NAME, local_time);

	return saveLogFilename;
}