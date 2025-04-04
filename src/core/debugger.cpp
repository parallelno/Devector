#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>

#include "core/debugger.h"
#include "utils/str_utils.h"
#include "utils/utils.h"

dev::Debugger::Debugger(Hardware& _hardware)
	:
	m_hardware(_hardware),
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
}

// UI thread
dev::Debugger::~Debugger()
{
	m_hardware.Request(Hardware::Req::DEBUG_ATTACH, { {"data", false} });
}

// Hardware thread.
// Has to be called after Hardware Reset, loading the rom file, fdd immage, attach/dettach debugger, 
// and other operations that change the Hardware states because this func stores the last state of Hardware
void dev::Debugger::Reset(bool _resetRecorder, 
	CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
	IO::State* _ioStateP, Display::State* _displayStateP)
{
	m_disasm.Reset();

	m_lastWritesAddrs.fill(uint32_t(LAST_RW_NO_DATA));
	m_lastReadsAddrs.fill(uint32_t(LAST_RW_NO_DATA));
	m_lastWritesIdx = 0;
	m_lastReadsIdx = 0;
	m_memLastRW.fill(0);

	m_traceLog.Reset();
	if (_resetRecorder) m_recorder.Reset(_cpuStateP, _memStateP, _ioStateP, _displayStateP);
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

			m_debugData.GetWatchpoints()->Check(Watchpoint::Access::R, globalAddr, val);

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

			// check if the memory is read-only
			auto memEdit = m_debugData.GetMemoryEdit(globalAddr);
			if (memEdit && memEdit->active && memEdit->readonly) {
				_memStateP->debug.write[i] = _memStateP->debug.beforeWrite[i];
				_memStateP->ramP->at(globalAddr) = _memStateP->debug.beforeWrite[i];
				continue;
			};

			m_disasm.MemWritesUpdate(globalAddr);

			m_debugData.GetWatchpoints()->Check(Watchpoint::Access::W, globalAddr, val);

			m_lastWritesAddrs[m_lastWritesIdx++] = globalAddr;
			m_lastWritesIdx %= LAST_RW_MAX;
		}
	}

	// code perf
	m_debugData.CheckCodePerfs(_cpuStateP->regs.pc.word, _cpuStateP->cc);

	auto break_ = false;

	// check scripts
	break_ |= m_debugData.GetScripts()->Check(_cpuStateP, _memStateP, _ioStateP, _displayStateP);
	
	// check watchpoint status
	break_ |= m_debugData.GetWatchpoints()->CheckBreak();

	// check breakpoints
	break_ |= m_debugData.GetBreakpoints()->Check(*_cpuStateP, *_memStateP);

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
		Reset(_reqDataJ["resetRecorder"], _cpuStateP, _memStateP, _ioStateP, _displayStateP);
		break;
	//////////////////
	// 
	// Recorder
	//
	/////////////////
	
	case Hardware::Req::DEBUG_RECORDER_RESET:
		m_recorder.Reset(_cpuStateP, _memStateP, _ioStateP, _displayStateP);
		break;

	case Hardware::Req::DEBUG_RECORDER_PLAY_FORWARD:
		m_recorder.PlayForward(_reqDataJ["frames"], _cpuStateP, _memStateP, _ioStateP, _displayStateP);
		break;

	case Hardware::Req::DEBUG_RECORDER_PLAY_REVERSE:
		m_recorder.PlayReverse(_reqDataJ["frames"], _cpuStateP, _memStateP, _ioStateP, _displayStateP);
		break;

	case Hardware::Req::DEBUG_RECORDER_GET_STATE_RECORDED:
		out = nlohmann::json{ {"states", m_recorder.GetStateRecorded() } };
		break;

	case Hardware::Req::DEBUG_RECORDER_GET_STATE_CURRENT:
		out = nlohmann::json{ {"states", m_recorder.GetStateCurrent() } };
		break;

	case Hardware::Req::DEBUG_RECORDER_SERIALIZE: {

		out = nlohmann::json{ {"data", nlohmann::json::binary(m_recorder.Serialize()) } };
		break;
	}
	case Hardware::Req::DEBUG_RECORDER_DESERIALIZE: {

		nlohmann::json::binary_t binaryData = _reqDataJ["data"].get<nlohmann::json::binary_t>();
		std::vector<uint8_t> data(binaryData.begin(), binaryData.end());

		m_recorder.Deserialize(data, _cpuStateP, _memStateP, _ioStateP, _displayStateP);
		break;
	}
	//////////////////
	// 
	// Breakpoints
	//
	/////////////////

	case Hardware::Req::DEBUG_BREAKPOINT_DEL_ALL:
		m_debugData.GetBreakpoints()->Clear();
		break;

	case Hardware::Req::DEBUG_BREAKPOINT_DEL:
		m_debugData.GetBreakpoints()->Del(_reqDataJ["addr"]);
		break;
		
	case Hardware::Req::DEBUG_BREAKPOINT_ADD: {
		Breakpoint::Data bpData{ _reqDataJ["data0"], _reqDataJ["data1"], _reqDataJ["data2"] };
		m_debugData.GetBreakpoints()->Add({ std::move(bpData), _reqDataJ["comment"] });
		break;
	}
	case Hardware::Req::DEBUG_BREAKPOINT_SET_STATUS:
		m_debugData.GetBreakpoints()->SetStatus( _reqDataJ["addr"], _reqDataJ["status"] );
		break;

	case Hardware::Req::DEBUG_BREAKPOINT_ACTIVE:
		m_debugData.GetBreakpoints()->SetStatus(_reqDataJ["addr"], Breakpoint::Status::ACTIVE);
		break;

	case Hardware::Req::DEBUG_BREAKPOINT_DISABLE:
		m_debugData.GetBreakpoints()->SetStatus(_reqDataJ["addr"], Breakpoint::Status::DISABLED);
		break;

	case Hardware::Req::DEBUG_BREAKPOINT_GET_STATUS:
		out = nlohmann::json{ {"status", static_cast<uint64_t>(m_debugData.GetBreakpoints()->GetStatus(_reqDataJ["addr"])) } };
		break;

	case Hardware::Req::DEBUG_BREAKPOINT_GET_UPDATES:
		out = nlohmann::json{ {"updates", static_cast<uint64_t>(m_debugData.GetBreakpoints()->GetUpdates()) } };
		break;

	case Hardware::Req::DEBUG_BREAKPOINT_GET_ALL:
		for(const auto& [addr, bp] : m_debugData.GetBreakpoints()->GetAll())
		{
			out.push_back( {
					{"data0", bp.data.data0}, 
					{"data1", bp.data.data1}, 
					{"data2", bp.data.data2}, 
					{"comment", bp.comment}
			});
		}
		break;

	//////////////////
	// 
	// Watchpoints
	//
	/////////////////

	case Hardware::Req::DEBUG_WATCHPOINT_DEL_ALL:
		m_debugData.GetWatchpoints()->Clear();
		break;

	case Hardware::Req::DEBUG_WATCHPOINT_DEL:
		m_debugData.GetWatchpoints()->Del(_reqDataJ["id"]);
		break;

	case Hardware::Req::DEBUG_WATCHPOINT_ADD: {
		Watchpoint::Data wpData{ _reqDataJ["data0"], _reqDataJ["data1"] };
		m_debugData.GetWatchpoints()->Add({ std::move(wpData), _reqDataJ["comment"] });
		break;
	}
	case Hardware::Req::DEBUG_WATCHPOINT_GET_UPDATES:
		out = nlohmann::json{ {"updates", static_cast<uint64_t>(m_debugData.GetWatchpoints()->GetUpdates()) } };
		break;

	case Hardware::Req::DEBUG_WATCHPOINT_GET_ALL:
		for (const auto& [id, wp] : m_debugData.GetWatchpoints()->GetAll())
		{
			out.push_back({
					{"data0", wp.data.data0},
					{"data1", wp.data.data1},
					{"comment", wp.comment}
				});
		}
		break;

	//////////////////
	// 
	// Memory Edits
	//
	/////////////////

	case Hardware::Req::DEBUG_MEMORY_EDIT_DEL_ALL:
		m_debugData.DelAllMemoryEdits();
		break;

	case Hardware::Req::DEBUG_MEMORY_EDIT_DEL:
		m_debugData.DelMemoryEdit(_reqDataJ["addr"]);
		break;

	case Hardware::Req::DEBUG_MEMORY_EDIT_ADD:
		m_debugData.SetMemoryEdit(_reqDataJ);
		break;
	
	case Hardware::Req::DEBUG_MEMORY_EDIT_GET:
	{
		auto memEdit = m_debugData.GetMemoryEdit(_reqDataJ["addr"]);
		if (memEdit)
		{
			out = { {"data", memEdit->ToJson()} };
		}
		break;
	}

	case Hardware::Req::DEBUG_MEMORY_EDIT_EXISTS:
		out = { {"data", m_debugData.GetMemoryEdit(_reqDataJ["addr"]) != nullptr } };
		break;

	//////////////////
	// 
	// Code Perfs
	//
	/////////////////

	case Hardware::Req::DEBUG_CODE_PERF_DEL_ALL:
		m_debugData.DelAllCodePerfs();
		break;

	case Hardware::Req::DEBUG_CODE_PERF_DEL:
		m_debugData.DelCodePerf(_reqDataJ["addr"]);
		break;

	case Hardware::Req::DEBUG_CODE_PERF_ADD:
		m_debugData.SetCodePerf(_reqDataJ);
		break;
	
	case Hardware::Req::DEBUG_CODE_PERF_GET:
	{
		auto codePerf = m_debugData.GetCodePerf(_reqDataJ["addr"]);
		if (codePerf)
		{
			out = { {"data", codePerf->ToJson()} };
		}
		break;
	}

	case Hardware::Req::DEBUG_CODE_PERF_EXISTS:
		out = { {"data", m_debugData.GetCodePerf(_reqDataJ["addr"]) != nullptr } };
		break;

	//////////////////
	// 
	// Scripts
	//
	/////////////////

	case Hardware::Req::DEBUG_SCRIPT_DEL_ALL:
		m_debugData.GetScripts()->Clear();
		break;

	case Hardware::Req::DEBUG_SCRIPT_DEL:
		m_debugData.GetScripts()->Del(_reqDataJ["id"]);
		break;

	case Hardware::Req::DEBUG_SCRIPT_ADD: {
		m_debugData.GetScripts()->Add(_reqDataJ);
		break;
	}
	case Hardware::Req::DEBUG_SCRIPT_GET_UPDATES:
		out = nlohmann::json{ {"updates", static_cast<uint64_t>(m_debugData.GetScripts()->GetUpdates()) } };
		break;

	case Hardware::Req::DEBUG_SCRIPT_GET_ALL:
		for (const auto& [id, script] : m_debugData.GetScripts()->GetAll())
		{
			out.push_back(script.ToJson());
		}
		break;
	
	default:
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
			auto breakpointStatus = m_debugData.GetBreakpoints()->GetStatus(addr);
			addr += m_disasm.AddCode(addr, cmd, breakpointStatus);
		}
	}

	while (!m_disasm.IsDone())
	{
		m_disasm.AddComment(addr);
		m_disasm.AddLabes(addr);

		uint32_t cmd = m_hardware.Request(Hardware::Req::GET_THREE_BYTES_RAM, { { "addr", addr } })->at("data");
		GlobalAddr globalAddr = m_hardware.Request(Hardware::Req::GET_GLOBAL_ADDR_RAM, { { "addr", addr } })->at("data");

		auto breakpointStatus = m_debugData.GetBreakpoints()->GetStatus(globalAddr);

		addr += m_disasm.AddCode(addr, cmd, breakpointStatus);
	}

	m_disasm.SetUpdated();
}

// UI thread
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