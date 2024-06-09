#include "Debugger.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>
#include "Utils/StrUtils.h"
#include "Utils/Utils.h"

dev::Debugger::Debugger(Hardware& _hardware)
	:
	m_hardware(_hardware),
	m_wpBreak(false),
	m_traceLog(),
	m_lastReadsAddrs(), m_lastWritesAddrs(),
	m_memLastRW(),
	m_lastReadsAddrsOld(), m_lastWritesAddrsOld(), 
	m_lastRWAddrsOut(), disasm(_hardware)
{
    Init();
}

void dev::Debugger::Init()
{
	m_checkBreakFunc = std::bind(&Debugger::CheckBreak, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
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
	disasm.Reset();

	m_lastWritesAddrs.fill(uint32_t(LAST_RW_NO_DATA));
	m_lastReadsAddrs.fill(uint32_t(LAST_RW_NO_DATA));
	m_lastWritesIdx = 0;
	m_lastReadsIdx = 0;
	m_memLastRW.fill(0);

	for (size_t i = 0; i < TRACE_LOG_SIZE; i++)
	{
		//m_traceLog[i].Clear();
	}
	m_traceLogIdx = 0;
	m_traceLogIdxViewOffset = 0;
}

// a hardware thread
void dev::Debugger::ReadInstr(
	const GlobalAddr _globalAddr, const uint8_t _val, const uint8_t _dataH, const uint8_t _dataL, const Addr _hl)
{
	disasm.MemRunsUpdate(_globalAddr);
	TraceLogUpdate(_globalAddr, _val, _dataH, _dataL, _hl);
}

// a hardware thread
void dev::Debugger::Read(
    const GlobalAddr _globalAddr, const uint8_t _val)
{
	disasm.MemReadsUpdate(_globalAddr);
    m_wpBreak |= CheckWatchpoint(Watchpoint::Access::R, _globalAddr, _val);

	std::lock_guard<std::mutex> mlock(m_lastRWMutex);
	m_lastReadsAddrs[m_lastReadsIdx++] = _globalAddr;
	m_lastReadsIdx %= LAST_RW_MAX;
}
// a hardware thread
void dev::Debugger::Write(const GlobalAddr _globalAddr, const uint8_t _val)
{
    disasm.MemWritesUpdate(_globalAddr);
    m_wpBreak |= CheckWatchpoint(Watchpoint::Access::W, _globalAddr, _val);
	
	std::lock_guard<std::mutex> mlock(m_lastRWMutex);
	m_lastWritesAddrs[m_lastWritesIdx++] = _globalAddr;
	m_lastWritesIdx %= LAST_RW_MAX;
}

//////////////////////////////////////////////////////////////
//
// Disasm
//
//////////////////////////////////////////////////////////////

// _instructionOffset defines the start address of the disasm. 
// _instructionOffset = 0 means the start address is the _addr, 
// _instructionOffset = -5 means the start address is 5 instructions prior the _addr, and vise versa.
void dev::Debugger::UpdateDisasm(const Addr _addr, const size_t _linesNum, const int _instructionOffset)
{
	if (_linesNum <= 0) return;
	size_t lines = dev::Max(_linesNum, Disasm::DISASM_LINES_MAX);
	disasm.Init(_linesNum);

	// calculate a new address that precedes the specified 'addr' by the instructionOffset
	Addr addr = disasm.GetAddr(_addr, _instructionOffset);

	if (_instructionOffset < 0 && addr == _addr)
	{
		// _instructionOffset < 0 means we want to disasm several intructions prior the _addr.
		// if the GetAddr output addr is equal to input _addr, that means 
		// there is no valid instructions fit into the range (_addr+_instructionOffset, _addr) 
		// and that means a data blob is ahead
		addr += (Addr)_instructionOffset;

		for (; disasm.GetLineIdx() < -_instructionOffset;)
		{
			disasm.AddComment(addr);
			disasm.AddLabes(addr);

			uint8_t db = m_hardware.Request(Hardware::Req::GET_BYTE_RAM, { { "addr", addr } })->at("data");
			auto breakpointStatus = GetBreakpointStatus(addr);
			addr += disasm.AddDb(addr, db, breakpointStatus);
		}
	}

	while (!disasm.IsDone())
	{
		disasm.AddComment(addr);
		disasm.AddLabes(addr);

		uint32_t cmd = m_hardware.Request(Hardware::Req::GET_THREE_BYTES_RAM, { { "addr", addr } })->at("data");

		GlobalAddr globalAddr = m_hardware.Request(Hardware::Req::GET_GLOBAL_ADDR_RAM, { { "addr", addr } })->at("data");
		auto breakpointStatus = GetBreakpointStatus(globalAddr);

		addr += disasm.AddCode(addr, cmd, breakpointStatus);
	}

	disasm.SetUpdated();
}

//////////////////////////////////////////////////////////////
//
// Tracelog
//
//////////////////////////////////////////////////////////////

// a hardware thread
void dev::Debugger::TraceLogUpdate(const GlobalAddr _globalAddr, 
	const uint8_t _opcode, const uint8_t _dataH, const uint8_t _dataL, const Addr _hl)
{
	// skip repeataive HLT
	/*if (_opcode == OPCODE_HLT && m_traceLog[m_traceLogIdx].m_opcode == OPCODE_HLT) {
		return;
	}

	m_traceLogIdx = --m_traceLogIdx % TRACE_LOG_SIZE;
	m_traceLog[m_traceLogIdx].m_globalAddr = _globalAddr;
	m_traceLog[m_traceLogIdx].m_opcode = _opcode;
	m_traceLog[m_traceLogIdx].m_dataL = _opcode != OPCODE_PCHL ? _dataL : _hl & 0xff;
	m_traceLog[m_traceLogIdx].m_dataH = _opcode != OPCODE_PCHL ? _dataH : _hl >> 8;*/
}

auto dev::Debugger::GetTraceLog(const int _offset, const size_t _lines, const size_t _filter)
-> const Disasm::Lines*
{
	//size_t filter = dev::Min(_filter, OPCODE_TYPE_MAX);
	//size_t offset = dev::Max(_offset, 0);

	//DisasmLines out;

	//for (int i = 0; i < offset; i++)
	//{
	//	m_traceLogIdxViewOffset = TraceLogNextLine(m_traceLogIdxViewOffset, _offset < 0, filter);
	//}

	//size_t idx = m_traceLogIdx + m_traceLogIdxViewOffset;
	//size_t idx_last = m_traceLogIdx + TRACE_LOG_SIZE - 1;
	//size_t line = 0;
	//size_t first_line_idx = TraceLogNearestForwardLine(m_traceLogIdx, filter);

	//for (; idx <= idx_last && line < _lines; idx++)
	//{
	//	auto globalAddr = m_traceLog[idx % TRACE_LOG_SIZE].m_globalAddr;
	//	if (globalAddr < 0) break;

	//	if (get_opcode_type(m_traceLog[idx % TRACE_LOG_SIZE].m_opcode) <= filter)
	//	{
	//		std::string str = m_traceLog[idx % TRACE_LOG_SIZE].ToStr();

	//		const Addr operand_addr = m_traceLog[idx % TRACE_LOG_SIZE].m_dataH << 8 | m_traceLog[idx % TRACE_LOG_SIZE].m_dataL;
	//		std::string constsS = LabelsToStr(operand_addr, LABEL_TYPE_ALL);

	//		DisasmLine lineDisasm(DisasmLine::Type::CODE, Addr(globalAddr), str, 0,0,0, constsS);
	//		out.emplace_back(std::move(lineDisasm));

	//		line++;
	//	}
	//}

	//return out;
	return nullptr;
}

auto dev::Debugger::TraceLogNextLine(const int _idxOffset, const bool _reverse, const size_t _filter) const
->int
{
	//size_t filter = dev::Min(_filter, OPCODE_TYPE_MAX);

	//size_t idx = m_traceLogIdx + _idxOffset;
	//size_t idx_last = m_traceLogIdx + TRACE_LOG_SIZE - 1;

	//int dir = _reverse ? -1 : 1;
	//// for a forward scrolling we need to go to the second line if we were not exactly at the filtered line
	//bool forward_second_search = false;
	//size_t first_line_idx = idx;

	//for (; idx >= m_traceLogIdx && idx <= idx_last; idx += dir)
	//{
	//	if (get_opcode_type(m_traceLog[idx % TRACE_LOG_SIZE].m_opcode) <= filter)
	//	{
	//		if ((!_reverse && !forward_second_search) ||
	//			(_reverse && idx == first_line_idx && !forward_second_search))
	//		{
	//			forward_second_search = true;
	//			continue;
	//		}
	//		else
	//		{
	//			return (int)(idx - m_traceLogIdx);
	//		}
	//	}
	//}

	//return _idxOffset; // fails to reach the next line
	return 0;
}

auto dev::Debugger::TraceLogNearestForwardLine(const size_t _idx, const size_t _filter) const
->int
{
	//size_t filter = _filter > OPCODE_TYPE_MAX ? OPCODE_TYPE_MAX : _filter;

	//size_t idx = _idx;
	//size_t idx_last = m_traceLogIdx + TRACE_LOG_SIZE - 1;

	//for (; idx >= m_traceLogIdx && idx <= idx_last; idx++)
	//{
	//	if (get_opcode_type(m_traceLog[idx % TRACE_LOG_SIZE].m_opcode) <= filter)
	//	{
	//		return (int)idx;
	//	}
	//}

	//return (int)_idx; // fails to reach the nearest line
	return 0;
}

//auto dev::Debugger::TraceLog::ToStr() const
//->std::string
//{
//	return GetMnemonic1(m_opcode, m_dataL, m_dataH);
//}

//void dev::Debugger::TraceLog::Clear()
//{
//	m_globalAddr = -1;
//	m_opcode = 0;
//	m_dataL = 0;
//	m_dataH = 0;
//}

//////////////////////////////////////////////////////////////
//
// Debug flow
//
//////////////////////////////////////////////////////////////

// m_hardware thread
bool dev::Debugger::CheckBreak(const Addr _addr, const uint8_t _mappingModeRam, const uint8_t _mappingPageRam)
{
	if (m_wpBreak)
	{
		m_wpBreak = false;
		ResetWatchpoints();
		return true;
	}

	auto break_ = CheckBreakpoints(_addr, _mappingModeRam, _mappingPageRam);

	return break_;
}

//////////////////////////////////////////////////////////////
//
// Breakpoints
//
//////////////////////////////////////////////////////////////

void dev::Debugger::SetBreakpointStatus(const Addr _addr, const Breakpoint::Status _status)
{
	{
		std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
		auto bpI = m_breakpoints.find(_addr);
		if (bpI != m_breakpoints.end()) {
			bpI->second.SetStatus(_status);
			return;
		}
	}
	AddBreakpoint(_addr);
}

void dev::Debugger::AddBreakpoint(const Addr _addr, 
	const uint8_t _mappingPages,
	const Breakpoint::Status _status,
	const bool _autoDel, const std::string& _comment)
{
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	auto bpI = m_breakpoints.find(_addr);
	if (bpI != m_breakpoints.end())
	{
		bpI->second.Update(_addr, _mappingPages, _status, _autoDel, _comment);
		return;
	}

	m_breakpoints.emplace(_addr, std::move(Breakpoint(_addr, _mappingPages, _status, _autoDel, _comment)));
}

void dev::Debugger::DelBreakpoint(const Addr _addr)
{
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	auto bpI = m_breakpoints.find(_addr);
	if (bpI != m_breakpoints.end())
	{
		m_breakpoints.erase(bpI);
	}
}

void dev::Debugger::DelBreakpoints()
{
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	m_breakpoints.clear();
}

bool dev::Debugger::CheckBreakpoints(const Addr _addr, const uint8_t _mappingModeRam, const uint8_t _mappingPageRam)
{
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	auto bpI = m_breakpoints.find(_addr);
	if (bpI == m_breakpoints.end()) return false;
	auto status = bpI->second.CheckStatus(_mappingModeRam, _mappingPageRam);
	if (bpI->second.GetData().autoDel) m_breakpoints.erase(bpI);
	return status;
}

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

auto dev::Debugger::GetBreakpointStatus(const Addr _addr)
-> const Breakpoint::Status
{
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	auto bpI = m_breakpoints.find(_addr);

	return bpI == m_breakpoints.end() ? Breakpoint::Status::DELETED : bpI->second.GetData().status;
}

//////////////////////////////////////////////////////////////
//
// Watchpoint
//
//////////////////////////////////////////////////////////////

void dev::Debugger::AddWatchpoint(
	const dev::Id _id, const Watchpoint::Access _access, 
	const GlobalAddr _globalAddr, const Watchpoint::Condition _cond,
	const uint16_t _value, const Watchpoint::Type _type, const int _len, const bool _active, const std::string& _comment)
{
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
	
	auto wpI = m_watchpoints.find(_id);
	if (wpI != m_watchpoints.end())
	{
		wpI->second.Update(_access, _globalAddr, _cond, _value, _type, _len, _active, _comment);
	}
	else {
		auto wp = Watchpoint(_access, _globalAddr, _cond, _value, _type, _len, _active, _comment);
		m_watchpoints.emplace(wp.GetId(), std::move(wp));
	}
}

void dev::Debugger::DelWatchpoint(const dev::Id _id)
{
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);

	auto bpI = m_watchpoints.find(_id);
	if (bpI != m_watchpoints.end())
	{
		m_watchpoints.erase(bpI);
	}
}

void dev::Debugger::DelWatchpoints()
{
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
	m_watchpoints.clear();
}

// a hardware thread
bool dev::Debugger::CheckWatchpoint(const Watchpoint::Access _access, const GlobalAddr _globalAddr, const uint8_t _value)
{
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
	
	auto wpI = std::find_if(m_watchpoints.begin(), m_watchpoints.end(), 
		[_access, _globalAddr, _value](Watchpoints::value_type& pair) 
		{
			return pair.second.Check(_access, _globalAddr, _value);
		});

	if (wpI == m_watchpoints.end()) return false;

	return true;
}

void dev::Debugger::ResetWatchpoints()
{
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
	for (auto& [id, watchpoint] : m_watchpoints)
	{
		watchpoint.Reset();
	}
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
// Requests
//
//////////////////////////////////////////////////////////////

void dev::Debugger::UpdateLastRW()
{
	// remove old stats
	for (int i = 0; i < m_lastReadsAddrsOld.size(); i++) 
	{
		auto globalAddrLastRead = m_lastReadsAddrsOld[i];
		if (globalAddrLastRead != LAST_RW_NO_DATA) {
			m_memLastRW[globalAddrLastRead] = 0;
		}
		auto globalAddrLastWrite = m_lastWritesAddrsOld[i];
		if (globalAddrLastWrite != LAST_RW_NO_DATA) {
			m_memLastRW[globalAddrLastWrite] = 0;
		}
	}

	// copy new reads stats
	std::lock_guard<std::mutex> mlock(m_lastRWMutex);
	uint16_t readsIdx = m_lastReadsIdx;
	for (auto globalAddr : m_lastReadsAddrs){
		if (globalAddr != LAST_RW_NO_DATA) 
		{
			auto val = m_memLastRW[globalAddr] & 0xFFFF0000; // remove reads, keep writes
			m_memLastRW[globalAddr] = val | static_cast<uint16_t>(LAST_RW_MAX - readsIdx) % LAST_RW_MAX;
		}
		readsIdx--;
	}

	// copy new writes stats
	uint16_t writesIdx = m_lastWritesIdx;
	for (auto globalAddr : m_lastWritesAddrs){
		if (globalAddr != LAST_RW_NO_DATA) 
		{
			auto val = m_memLastRW[globalAddr] & 0x0000FFFF; // remove writes, keep reads
			m_memLastRW[globalAddr] = val | (static_cast<uint16_t>(LAST_RW_MAX - writesIdx) % LAST_RW_MAX)<<16;
		}
		writesIdx--;
	}
	
	m_lastReadsAddrsOld = m_lastReadsAddrs;
	m_lastWritesAddrsOld = m_lastWritesAddrs;
}

auto dev::Debugger::GetLastRW() -> const MemLastRW* { return &m_memLastRW; }