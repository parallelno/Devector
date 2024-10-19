#include "core/hardware.h"
#include "utils/str_utils.h"
#include "core/disasm.h"

dev::Hardware::Hardware(const std::wstring& _pathBootData, 
		const std::wstring& _pathRamDiskData, const bool _ramDiskClearAfterRestart)
	:
	m_status(Status::STOP),
	m_memory(_pathBootData, _pathRamDiskData, _ramDiskClearAfterRestart),
	m_keyboard(),
	m_timer(),
	m_ay(),
	m_aywrapper(m_ay),
	m_audio(m_timer, m_aywrapper),
	m_fdc(),
	m_io(m_keyboard, m_memory, m_timer, m_ay, m_fdc),
	m_cpu(
		m_memory,
		std::bind(&IO::PortIn, &m_io, std::placeholders::_1),
		std::bind(&IO::PortOut, &m_io, std::placeholders::_1, std::placeholders::_2)),
	m_display(m_memory, m_io)
{
	Init();
	m_executionThread = std::thread(&Hardware::Execution, this);
}

dev::Hardware::~Hardware()
{
	Request(Hardware::Req::EXIT);
	m_executionThread.join();
}

// when HW needs Reset
void dev::Hardware::Init()
{
	m_memory.Init();
	m_display.Init();
	m_io.Init();
}


void dev::Hardware::AttachDebugFuncs(DebugFunc _debugFunc, DebugReqHandlingFunc _debugReqHandlingFunc)
{ 
	Debug = _debugFunc;
	DebugReqHandling = _debugReqHandlingFunc;
}

// outputs true if the execution breaks
bool dev::Hardware::ExecuteInstruction()
{
	// mem debug init
	m_memory.DebugInit();

	do
	{
		m_display.Rasterize();
		m_cpu.ExecuteMachineCycle(m_display.IsIRQ());
		m_audio.Clock(2, m_io.GetBeeper());

	} while (!m_cpu.IsInstructionExecuted());

	// debug per instruction
	if (m_debugAttached && Debug(m_cpu.GetStateP(), m_memory.GetStateP(), m_io.GetStateP(), m_display.GetStateP()) ) {
		return true;
	}

	if (m_memory.IsException())
	{
		dev::Log("ERROR: more than one Ram-disk has mapping enabled");
		return true;
	}

	ReqHandling();

	return false;
}

// TODO:
//1. load the playback data via the Load main menu. auto play it from the first frame. Save the playback in the recorder window.

//1. check how the playback adds data after reverse playback. make sure that it s not combining two frames ino one state
// 
//1. when we reverse, or play forward, we need to handle the special case - the last frame.
//		if we at the last mem updates, reverse operation has to resore that memory, then update the state.
//		if we at the start of the frame in the last frame even there is some memory updates, we first do a step back, then restore the memory, then restore the state
// 
//1. if we play forward we also has to handle the last frame with two sub states - the start of the frame and the middle frame where we stoppped recording
//1. for the play forward we need to write into the memory what instructions did. for it we need to store what instructions wrote. when we play forward the steps: store the memory before write, check if this is the new frame. if so, advance to the next frame and store the state. store into a special array what an instruction wrote and the address.

//1. reload, reset, update the palette, and other non-hardware-initiated operations have to reset the playback history

void dev::Hardware::Execution()
{    
	while (m_status != Status::EXIT)
	{
		auto startCC = m_cpu.GetCC();
		auto startFrame = m_display.GetFrameNum();
		auto startTime = std::chrono::system_clock::now();

		auto expectedTime = std::chrono::system_clock::now();

		while (m_status == Status::RUN)
		{   
			auto frameNum = m_display.GetFrameNum();
			
			do // rasterizes a frame
			{
				if (ExecuteInstruction())
				{
					Stop();
					break;
				};

			} while (m_status == Status::RUN && m_display.GetFrameNum() == frameNum);

			// vsync
			if (m_status == Status::RUN)
			{
				expectedTime = expectedTime + m_execDelays[static_cast<int>(m_execSpeed)];

				if (std::chrono::system_clock::now() < expectedTime){
					std::this_thread::sleep_until(expectedTime);
				}
			}
		}

		// print out the break statistics
		auto elapsedCC = m_cpu.GetCC() - startCC;
		if (elapsedCC) {
			auto elapsedFrames = m_display.GetFrameNum() - startFrame;
			std::chrono::duration<int64_t, std::nano> elapsedTime = std::chrono::system_clock::now() - startTime;
			double timeDurationSec = elapsedTime.count() / 1000000000.0;
			dev::Log("Break: elapsed cpu cycles: {}, elapsed frames: {}, elapsed seconds: {}", elapsedCC, elapsedFrames, timeDurationSec);
		}

		while (m_status == Status::STOP)
		{
			ReqHandling(true);
		}
	}
}

// UI thread. It return when the request fulfilled
auto dev::Hardware::Request(const Req _req, const nlohmann::json& _dataJ)
-> Result<nlohmann::json>
{
	m_reqs.push({ _req, _dataJ });
	return m_reqRes.pop();
}

// internal thread
void dev::Hardware::ReqHandling(const bool _waitReq)
{
	if (!m_reqs.empty() || _waitReq)
	{        
		auto result = m_reqs.pop();

		const auto& [req, dataJ] = *result;

		switch (req)
		{
		case Req::RUN:
			Run();
			m_reqRes.emplace({});
			break;

		case Req::STOP:
			Stop();
			m_reqRes.emplace({});
			break;

		case Req::IS_RUNNING:
			m_reqRes.emplace({
				{"isRunning", m_status == Status::RUN},
				});
			break;

		case Req::EXIT:
			m_status = Status::EXIT;
			m_reqRes.emplace({});
			break;

		case Req::RESET:
			Reset();
			m_reqRes.emplace({});
			break;

		case Req::RESTART:
			Restart();
			m_reqRes.emplace({});
			break;

		case Req::EXECUTE_INSTR:
			ExecuteInstruction();
			m_reqRes.emplace({});
			break;

		case Req::EXECUTE_FRAME_NO_BREAKS:
		{
			ExecuteFrameNoBreaks();
			m_reqRes.emplace({});
			break;
		}
		case Req::GET_CC:
			m_reqRes.emplace({
				{"cc", m_cpu.GetCC() },
				});
			break;

		case Req::GET_REGS:
		{
			auto regsJ = GetRegs();
			m_reqRes.emplace(regsJ);
			break;
		}
		case Req::GET_REG_PC:
			m_reqRes.emplace({
				{"pc", m_cpu.GetPC() },
				});
			break;

		case Req::GET_RUSLAT_HISTORY:
			m_reqRes.emplace({
				{"data", m_io.GetRusLatHistory()},
				});
			break;

		case Req::GET_PALETTE:
		{
			auto data = m_io.GetPalette();
			m_reqRes.emplace({
				{"low", data->low},
				{"hi", data->hi},
				});
			break;
		}
		case Req::GET_IO_PORTS:
		{
			auto data = m_io.GetPorts();
			m_reqRes.emplace({
				{"data", data->data},
				});
			break;
		}

		case Req::GET_IO_PALETTE_COMMIT_TIME:
		{
			auto data = m_io.GetPaletteCommitTime();
			m_reqRes.emplace({
				{"paletteCommitTime", data},
				});
			break;
		}

		case Req::SET_IO_PALETTE_COMMIT_TIME:
		{
			m_io.SetPaletteCommitTime(dataJ["paletteCommitTime"]);
			m_reqRes.emplace({});
			break;
		}

		case Req::GET_DISPLAY_BORDER_LEFT:
		{
			auto data = m_display.GetBorderLeft();
			m_reqRes.emplace({
				{"borderLeft", data},
				});
			break;
		}

		case Req::SET_DISPLAY_BORDER_LEFT:
		{
			m_display.SetBorderLeft(dataJ["borderLeft"]);
			m_reqRes.emplace({});
			break;
		}

		case Req::GET_DISPLAY_IRQ_COMMIT_PXL:
		{
			auto data = m_display.GetIrqCommitPxl();
			m_reqRes.emplace({
				{"irqCommitPxl", data},
				});
			break;
		}

		case Req::SET_DISPLAY_IRQ_COMMIT_PXL:
		{
			m_display.SetIrqCommitPxl(dataJ["irqCommitPxl"]);
			m_reqRes.emplace({});
			break;
		}

		case Req::GET_IO_DISPLAY_MODE:
			m_reqRes.emplace({
				{"data", m_io.GetDisplayMode()},
				});
			break;

		case Req::GET_BYTE_GLOBAL:
			m_reqRes.emplace(GetByteGlobal(dataJ));
			break;

		case Req::GET_BYTE_RAM:
			m_reqRes.emplace(GetByte(dataJ, Memory::AddrSpace::RAM));
			break;

		case Req::GET_THREE_BYTES_RAM:
			m_reqRes.emplace(Get3Bytes(dataJ, Memory::AddrSpace::RAM));
			break;

		case Req::GET_WORD_STACK:
			m_reqRes.emplace(GetWord(dataJ, Memory::AddrSpace::STACK));
			break;

		case Req::GET_DISPLAY_DATA:
			m_reqRes.emplace({
				{"rasterLine", m_display.GetRasterLine()},
				{"rasterPixel", m_display.GetRasterPixel()},
				{"frameNum", m_display.GetFrameNum()},
				});
			break;

		case Req::GET_MEMORY_MAPPING:
			m_reqRes.emplace({
				{"mapping", m_memory.GetState().update.mapping.data},
				{"ramdiskIdx", m_memory.GetState().update.ramdiskIdx},
				});
			break;
		case Req::GET_GLOBAL_ADDR_RAM:
			m_reqRes.emplace({
				{"data", m_memory.GetGlobalAddr(dataJ["addr"], Memory::AddrSpace::RAM)}
				});
			break;

		case Req::GET_FDC_INFO: {
			auto info = m_fdc.GetFdcInfo();
			m_reqRes.emplace({
				{"drive", info.drive},
				{"side", info.side},
				{"track", info.track},
				{"lastS", info.lastS},
				{"wait", info.irq},
				{"cmd", info.cmd},
				{"rwLen", info.rwLen},
				{"position", info.position},
				});
			break;
		}

		case Req::GET_FDD_INFO: {
			auto info = m_fdc.GetFddInfo(dataJ["driveIdx"]);
			m_reqRes.emplace({
				{"path", dev::StrWToStr(info.path)},
				{"updated", info.updated},
				{"reads", info.reads},
				{"writes", info.writes},
				{"mounted", info.mounted},
				});
			break;
		}
		
		case Req::GET_FDD_IMAGE:
			m_reqRes.emplace({
				{"data", m_fdc.GetFddImage(dataJ["driveIdx"])},
				});
			break;

		case Req::GET_STEP_OVER_ADDR:
			m_reqRes.emplace({
				{"data", GetStepOverAddr()},
				});
			break;

		case Req::GET_IO_PORTS_IN_DATA:
		{
			auto portsData = m_io.GetPortsInData();
			m_reqRes.emplace({
				{"data0", portsData->data0},
				{"data1", portsData->data1},
				{"data2", portsData->data2},
				{"data3", portsData->data3},
				{"data4", portsData->data4},
				{"data5", portsData->data5},
				{"data6", portsData->data6},
				{"data7", portsData->data7},
				});
			break;
		}
		case Req::GET_IO_PORTS_OUT_DATA:
		{
			auto portsData = m_io.GetPortsOutData();
			m_reqRes.emplace({
				{"data0", portsData->data0},
				{"data1", portsData->data1},
				{"data2", portsData->data2},
				{"data3", portsData->data3},
				{"data4", portsData->data4},
				{"data5", portsData->data5},
				{"data6", portsData->data6},
				{"data7", portsData->data7},
				});
			break;
		}
		case Req::SET_MEM:
			m_memory.SetRam(dataJ["addr"], dataJ["data"]);
			m_reqRes.emplace({});
			break;

		case Req::SET_CPU_SPEED:
		{
			int speed = dataJ["speed"];
			speed = std::clamp(speed, 0, int(sizeof(m_execDelays) - 1));
			m_execSpeed = static_cast<ExecSpeed>(speed);
			if (m_execSpeed == ExecSpeed::_20PERCENT) { m_audio.Mute(true); }
			else { m_audio.Mute(false); }
			m_reqRes.emplace({});
			break;
		}
		case Req::IS_MEMROM_ENABLED:
			m_reqRes.emplace({
				{"data", m_memory.IsRomEnabled() },
				});
			break;             

		case Req::KEY_HANDLING:
		{
			auto op = m_io.GetKeyboard().KeyHandling(dataJ["scancode"], dataJ["action"]);
			if (op == Keyboard::Operation::RESET) {
				Reset();
			}
			else if (op == Keyboard::Operation::RESTART) {
				Restart();
			}
			m_reqRes.emplace({});
		}
			break;

		case Req::GET_SCROLL_VERT:
			m_reqRes.emplace({
				{"scrollVert", m_display.GetScrollVert()}
				});
			break;

		case Req::LOAD_FDD:
			m_fdc.Mount(dataJ["driveIdx"], dataJ["data"], dev::StrToStrW(dataJ["path"]));
			m_reqRes.emplace({});
			break;

		case Req::RESET_UPDATE_FDD:
			m_fdc.ResetUpdate(dataJ["driveIdx"]);
			m_reqRes.emplace({});
			break;

		case Req::DEBUG_ATTACH:
			m_debugAttached = dataJ["data"];
			m_reqRes.emplace({});
			break;

		default:
			m_reqRes.emplace(
				DebugReqHandling(req, dataJ, m_cpu.GetStateP(), m_memory.GetStateP(), m_io.GetStateP(), m_display.GetStateP()) );
		}
	}
}

void dev::Hardware::Reset()
{
	Init();
	m_cpu.Reset();
	m_audio.Reset();
}

void dev::Hardware::Restart()
{
	m_cpu.Reset();
	m_audio.Reset();
	m_memory.Restart();
}

void dev::Hardware::Stop()
{
	m_status = Status::STOP;
	m_audio.Pause(true);
}

// to continue execution
void dev::Hardware::Run()
{
	m_status = Status::RUN;
	m_audio.Pause(false);
}

auto dev::Hardware::GetRegs() const
-> nlohmann::json
{
	auto& cpuState = m_cpu.GetState();

	nlohmann::json out {
		{"cc", cpuState.cc },
		{"pc", cpuState.regs.pc.word },
		{"sp", cpuState.regs.sp.word },
		{"af", cpuState.regs.psw.af.word },
		{"bc", cpuState.regs.bc.word },
		{"de", cpuState.regs.de.word },
		{"hl", cpuState.regs.hl.word },
		{"ints", cpuState.ints.data },
	};
	return out;
}

auto dev::Hardware::GetByteGlobal(const nlohmann::json _globalAddrJ)
-> nlohmann::json
{
	GlobalAddr globalAddr = _globalAddrJ["globalAddr"];
	uint8_t val = m_memory.GetRam()->at(globalAddr);
	nlohmann::json out = {
		{"data", val}
	};
	return out;
}

auto dev::Hardware::GetByte(const nlohmann::json _addrJ, const Memory::AddrSpace _addrSpace)
-> nlohmann::json
{
	Addr addr = _addrJ["addr"];
	nlohmann::json out = {
		{"data", m_memory.GetByte(addr, _addrSpace)}
	};
	return out;
}

auto dev::Hardware::Get3Bytes(const nlohmann::json _addrJ, const Memory::AddrSpace _addrSpace)
-> nlohmann::json
{
	Addr addr = _addrJ["addr"];
	auto data = m_memory.GetByte(addr, _addrSpace) |
		m_memory.GetByte(addr + 1, _addrSpace) << 8 |
		m_memory.GetByte(addr + 2, _addrSpace) << 16;

	nlohmann::json out = {
		{"data", data }
	};
	return out;
}

auto dev::Hardware::GetWord(const nlohmann::json _addrJ, const Memory::AddrSpace _addrSpace)
-> nlohmann::json
{
	Addr addr = _addrJ["addr"];
	auto data = m_memory.GetByte(addr + 1, _addrSpace) << 8 | m_memory.GetByte(addr, _addrSpace);

	nlohmann::json out = {
		{"data", data}
	};
	return out;
}

auto dev::Hardware::GetRam() const
-> const Memory::Ram*
{
	return m_memory.GetRam();
}

// UI thread
auto dev::Hardware::GetFrame(const bool _vsync)
->const Display::FrameBuffer*
{
	return m_display.GetFrame(_vsync);
}

void dev::Hardware::ExecuteFrameNoBreaks()
{
	auto frameNum = m_display.GetFrameNum();
	do {
		ExecuteInstruction();
	} while (m_display.GetFrameNum() == frameNum);
}

auto dev::Hardware::GetStepOverAddr()
-> const Addr
{
	auto pc = m_cpu.GetPC();
	auto sp = m_cpu.GetSP();
	auto opcode = m_memory.GetByte(pc);

	switch (dev::GetOpcodeType(opcode)) 
	{
	case OPTYPE_JMP:
		return m_memory.GetByte(pc + 2) << 8 | m_memory.GetByte(pc + 1);
	case OPTYPE_RET:
		return m_memory.GetByte(sp + 1, Memory::AddrSpace::STACK) << 8 | m_memory.GetByte(sp, Memory::AddrSpace::STACK);
	case OPTYPE_PCH:
		return m_cpu.GetHL();
	case OPTYPE_RST:
		return opcode - CpuI8080::OPCODE_RST0;
	}
	return pc + dev::GetCmdLen(opcode);
}