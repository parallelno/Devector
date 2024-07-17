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
	m_lastReadsAddrs(), m_lastWritesAddrs(),
	m_memLastRW(),
	m_lastReadsAddrsOld(), m_lastWritesAddrsOld(), 
	m_debugData(_hardware), m_disasm(_hardware, m_debugData),
	m_traceLog(m_debugData),
	m_lastRWAddrsOut()
{
	Init();
}

void dev::Debugger::Init()
{
	m_debugFunc = std::bind(&Debugger::Debug, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

	m_breakpoints.clear();
	m_watchpoints.clear();

	m_hardware.Request(Hardware::Req::RUN);
}

// UI thread
void dev::Debugger::Attach(const bool _attach)
{
	if (m_attached != _attach)
	{
		if (_attach)
		{
			bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");

			if (isRunning) { m_hardware.Request(Hardware::Req::STOP); }
			Reset();
			if (isRunning) { m_hardware.Request(Hardware::Req::RUN); }
		}

		m_hardware.AttachDebug(_attach ? &m_debugFunc : nullptr);
		m_attached = _attach;
	}
}

// UI thread
dev::Debugger::~Debugger()
{
	Attach(false);
}

// UI thread. Call while Hardware is stopped 
// Has to be called after Hardware Reset and loading Rom because it stores the last state of Hardware
void dev::Debugger::Reset()
{
	m_disasm.Reset();

	m_lastWritesAddrs.fill(uint32_t(LAST_RW_NO_DATA));
	m_lastReadsAddrs.fill(uint32_t(LAST_RW_NO_DATA));
	m_lastWritesIdx = 0;
	m_lastReadsIdx = 0;
	m_memLastRW.fill(0);

	m_traceLog.Reset();
	m_reverse.SetStatus(Reverse::STATUS_RESET);
}

//////////////////////////////////////////////////////////////
//
// Debug call from the Hardware thread
//
//////////////////////////////////////////////////////////////

// Hardware thread
bool dev::Debugger::Debug(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
	IO::State* _ioStateP, Display::State* _displayStateP)
{
	// instruction check
	m_disasm.MemRunsUpdate(_memStateP->debug.instrGlobalAddr);

	// reads check
	{
		std::lock_guard<std::mutex> mlock(m_lastRWMutex);

		for (int i = 0; i < _memStateP->debug.readLen; i++)
		{
			GlobalAddr globalAddr = _memStateP->debug.readGlobalAddr[i];
			uint8_t val = _memStateP->debug.read[i];

			m_disasm.MemReadsUpdate(globalAddr);

			m_wpBreak |= CheckWatchpoint(Watchpoint::Access::R, globalAddr, val);

			m_lastReadsAddrs[m_lastReadsIdx++] = globalAddr;
			m_lastReadsIdx %= LAST_RW_MAX;
		}
	}

	// writes check
	{
		std::lock_guard<std::mutex> mlock(m_lastRWMutex);

		for (int i = 0; i < _memStateP->debug.writeLen; i++)
		{
			GlobalAddr globalAddr = _memStateP->debug.writeGlobalAddr[i];
			uint8_t val = _memStateP->debug.write[i];

			m_disasm.MemWritesUpdate(globalAddr);

			m_wpBreak |= CheckWatchpoint(Watchpoint::Access::W, globalAddr, val);

			m_lastWritesAddrs[m_lastWritesIdx++] = globalAddr;
			m_lastWritesIdx %= LAST_RW_MAX;
		}
	}

	auto break_ = false;
	if (m_wpBreak)
	{
		m_wpBreak = false;
		ResetWatchpoints();
		break_ |= true;
	}
	break_ |= CheckBreakpoints(*_cpuStateP, *_memStateP);

	m_traceLog.Update(*_cpuStateP, *_memStateP);	
	m_reverse.Update(_cpuStateP, _memStateP, _ioStateP, _displayStateP);

	return break_;
}

//////////////////////////////////////////////////////////////
//
// Disasm
//
//////////////////////////////////////////////////////////////

// _instructionOffset defines the start address of the m_disasm. 
// _instructionOffset = 0 means the start address is the _addr, 
// _instructionOffset = -5 means the start address is 5 instructions prior the _addr, and vise versa.
void dev::Debugger::UpdateDisasm(const Addr _addr, const size_t _linesNum, const int _instructionOffset)
{
	if (_linesNum <= 0) return;
	size_t lines = dev::Max(_linesNum, Disasm::DISASM_LINES_MAX);
	m_disasm.Init(_linesNum);

	// calculate a new address that precedes the specified 'addr' by the instructionOffset
	Addr addr = m_disasm.GetAddr(_addr, _instructionOffset);

	if (_instructionOffset < 0 && addr == _addr)
	{
		// _instructionOffset < 0 means we want to disasm several intructions prior the _addr.
		// if the GetAddr output addr is equal to input _addr, that means 
		// there is no valid instructions fit into the range (_addr+_instructionOffset, _addr) 
		// and that means a data blob is ahead
		addr += (Addr)_instructionOffset;

		for (; m_disasm.GetLineIdx() < -_instructionOffset;)
		{
			m_disasm.AddComment(addr);
			m_disasm.AddLabes(addr);

			uint8_t db = m_hardware.Request(Hardware::Req::GET_BYTE_RAM, { { "addr", addr } })->at("data");
			uint32_t cmd = 0x1000 | db; // opcode 0x10 is used as a placeholder
			auto breakpointStatus = GetBreakpointStatus(addr);
			addr += m_disasm.AddCode(addr, cmd, breakpointStatus);
		}
	}

	while (!m_disasm.IsDone())
	{
		m_disasm.AddComment(addr);
		m_disasm.AddLabes(addr);

		uint32_t cmd = m_hardware.Request(Hardware::Req::GET_THREE_BYTES_RAM, { { "addr", addr } })->at("data");
		GlobalAddr globalAddr = m_hardware.Request(Hardware::Req::GET_GLOBAL_ADDR_RAM, { { "addr", addr } })->at("data");

		auto breakpointStatus = GetBreakpointStatus(globalAddr);

		addr += m_disasm.AddCode(addr, cmd, breakpointStatus);
	}

	m_disasm.SetUpdated();
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
			bpI->second.status = _status;
			return;
		}
	}
	AddBreakpoint(_addr);
}

void dev::Debugger::AddBreakpoint( const Addr _addr, const Breakpoint::MemPages _memPages,
	const Breakpoint::Status _status, const bool _autoDel,
	const Breakpoint::Operand _op, const dev::Condition _cond, 
	const size_t _val, const std::string& _comment)
{
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	auto bpI = m_breakpoints.find(_addr);
	if (bpI != m_breakpoints.end())
	{
		bpI->second.Update(_addr, _memPages, _status, _autoDel, _op, _cond, _val, _comment);
		return;
	}

	m_breakpoints.emplace(_addr, std::move(
		Breakpoint(_addr, _memPages, _status, _autoDel, _op, _cond, _val, _comment)));
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

bool dev::Debugger::CheckBreakpoints(const CpuI8080::State& _cpuState, const Memory::State& _memState)
{
	std::lock_guard<std::mutex> mlock(m_breakpointsMutex);
	auto bpI = m_breakpoints.find(_cpuState.regs.pc.word);
	if (bpI == m_breakpoints.end()) return false;
	auto status = bpI->second.CheckStatus(_cpuState, _memState);
	if (bpI->second.autoDel) m_breakpoints.erase(bpI);
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

	return bpI == m_breakpoints.end() ? Breakpoint::Status::DELETED : bpI->second.status;
}

//////////////////////////////////////////////////////////////
//
// Watchpoint
//
//////////////////////////////////////////////////////////////

void dev::Debugger::AddWatchpoint(
	const dev::Id _id, const Watchpoint::Access _access, 
	const GlobalAddr _globalAddr, const dev::Condition _cond,
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

// Hardware thread
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