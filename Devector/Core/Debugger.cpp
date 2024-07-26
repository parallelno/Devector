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

	Hardware::DebugFunc debugFunc = std::bind(&Debugger::Debug, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

	Hardware::DebugReqHandlingFunc debugReqHandlingFunc = std::bind(&Debugger::DebugReqHandling, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);

	m_hardware.AttachDebugFuncs(debugFunc, debugReqHandlingFunc);

	m_breakpoints.Clear();
	m_watchpoints.clear();
}

// UI thread
dev::Debugger::~Debugger()
{
	m_hardware.Request(Hardware::Req::DEBUG_ATTACH, { {"data", false} });
}

// Hardware thread.
// Has to be called after Hardware Reset, loading the rom file, fdd immage, attach/dettach debugger, 
// and other operations that change the Hardware states because this func stores the last state of Hardware
void dev::Debugger::Reset(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
	IO::State* _ioStateP, Display::State* _displayStateP)
{
	m_disasm.Reset();

	m_lastWritesAddrs.fill(uint32_t(LAST_RW_NO_DATA));
	m_lastReadsAddrs.fill(uint32_t(LAST_RW_NO_DATA));
	m_lastWritesIdx = 0;
	m_lastReadsIdx = 0;
	m_memLastRW.fill(0);

	m_traceLog.Reset();
	m_recorder.Reset(_cpuStateP, _memStateP, _ioStateP, _displayStateP);
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

	// check watchpoints
	auto break_ = false;
	if (m_wpBreak)
	{
		m_wpBreak = false;
		ResetWatchpoints();
		break_ |= true;
	}

	// check breakpoints
	break_ |= m_breakpoints.Check(*_cpuStateP, *_memStateP);

	// tracelog
	m_traceLog.Update(*_cpuStateP, *_memStateP);	

	// recorder
	m_recorder.Update(_cpuStateP, _memStateP, _ioStateP, _displayStateP);

	return break_;
}

// Hardware thread
auto dev::Debugger::DebugReqHandling(Hardware::Req _req, nlohmann::json _reqDataJ,
	CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
	IO::State* _ioStateP, Display::State* _displayStateP)
-> nlohmann::json
{
	nlohmann::json out;

	switch (_req)
	{
	case Hardware::Req::DEBUG_RESET:
		Reset(_cpuStateP, _memStateP, _ioStateP, _displayStateP);
		break;

	case Hardware::Req::DEBUG_PLAYBACK_REVERSE:
		m_recorder.PlaybackReverse(_reqDataJ["frames"], _cpuStateP, _memStateP, _ioStateP, _displayStateP);
		break;

	case Hardware::Req::DEBUG_BREAKPOINTS_DEL:
		m_breakpoints.Clear();
		break;

	case Hardware::Req::DEBUG_BREAKPOINT_DEL:
		m_breakpoints.Del(_reqDataJ["addr"]);
		break;
		
	case Hardware::Req::DEBUG_BREAKPOINT_ADD: {
		Breakpoint::Data bpData{ _reqDataJ["data0"], _reqDataJ["data1"], _reqDataJ["data2"] };
		m_breakpoints.Add({ std::move(bpData), _reqDataJ["comment"] });
		break;
	}
	case Hardware::Req::DEBUG_BREAKPOINT_SET_STATUS:
		m_breakpoints.SetStatus( _reqDataJ["addr"], _reqDataJ["status"] );
		break;

	case Hardware::Req::DEBUG_BREAKPOINT_ACTIVE:
		m_breakpoints.SetStatus(_reqDataJ["addr"], Breakpoint::Status::ACTIVE);
		break;

	case Hardware::Req::DEBUG_BREAKPOINT_DISABLE:
		m_breakpoints.SetStatus(_reqDataJ["addr"], Breakpoint::Status::DISABLED);
		break;

	case Hardware::Req::DEBUG_BREAKPOINT_GET_STATUS: {
		out = nlohmann::json{ {"status", static_cast<uint64_t>(m_breakpoints.GetStatus(_reqDataJ["addr"])) } };
		break;
	}
	case Hardware::Req::DEBUG_BREAKPOINT_GET_UPDATES: {
		out = nlohmann::json{ {"updates", static_cast<uint64_t>(m_breakpoints.GetUpdates()) } };
		break;
	}
	case Hardware::Req::DEBUG_BREAKPOINT_GET_ALL:
		for(const auto& [addr, bp] : m_breakpoints.GetAll())
		{
			out.push_back( {
					{"data0", bp.data.data0}, 
					{"data1", bp.data.data1}, 
					{"data2", bp.data.data2}, 
					{"comment", bp.comment}
			});
		}
		break;
	}

	return out;
}

//////////////////////////////////////////////////////////////
//
// Disasm
//
//////////////////////////////////////////////////////////////

// _instructionOffset defines the start address of the m_disasm. 
// _instructionOffset = 0 means the start address is the _addr, 
// _instructionOffset = -5 means the start address is 5 instructions prior the _addr, and vise versa.
// UI thread
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
			auto breakpointStatus = m_breakpoints.GetStatus(addr);
			addr += m_disasm.AddCode(addr, cmd, breakpointStatus);
		}
	}

	while (!m_disasm.IsDone())
	{
		m_disasm.AddComment(addr);
		m_disasm.AddLabes(addr);

		uint32_t cmd = m_hardware.Request(Hardware::Req::GET_THREE_BYTES_RAM, { { "addr", addr } })->at("data");
		GlobalAddr globalAddr = m_hardware.Request(Hardware::Req::GET_GLOBAL_ADDR_RAM, { { "addr", addr } })->at("data");

		auto breakpointStatus = m_breakpoints.GetStatus(globalAddr);

		addr += m_disasm.AddCode(addr, cmd, breakpointStatus);
	}

	m_disasm.SetUpdated();
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
	std::lock_guard<std::mutex> mlock(m_watchpointsMutex);
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