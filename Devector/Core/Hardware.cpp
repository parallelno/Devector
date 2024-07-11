#include "Hardware.h"
#include "Utils/StrUtils.h"
#include "Core/Disasm.h"

dev::Hardware::Hardware(const std::wstring& _pathBootData)
	:
	m_status(Status::STOP),
	m_memory(_pathBootData),
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
	// redundant. Debugg destructor is doing it
	//m_checkBreak.store(nullptr);
	//m_cpu.AttachDebugOnReadInstr(nullptr);
	//m_cpu.AttachDebugOnRead(nullptr);
	//m_cpu.AttachDebugOnWrite(nullptr);

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

void dev::Hardware::ExecuteInstruction()
{
	do
	{
		m_cpu.ExecuteMachineCycle(m_display.IsIRQ());
		m_audio.Clock(2, m_io.GetBeeper());
		m_display.Rasterize();

	} while (!m_cpu.IsInstructionExecuted());
}

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
			
			// rasterizes a frame
			do {
				auto TraceLogUpdate = m_traceLogUpdate.load();
				if (TraceLogUpdate)
				{
				   // TODO: provide internal states of io, fdc, etc components into the trace log update
					// (*TraceLogUpdate)(m_cpu.GetState(), m_memory.GetState(), m_);
				}
				ExecuteInstruction();
				ReqHandling();

				auto CheckBreak = m_checkBreak.load();
				if (CheckBreak && (*CheckBreak)(m_cpu.GetState(), m_memory.GetState()))				{
					Stop();
					break;
				}
				if (m_memory.IsRamDiskMappingCollision()) {
					Stop();
					break;
				}

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

		auto elapsedCC = m_cpu.GetCC() - startCC;
		if (elapsedCC) {
			auto elapsedFrames = m_display.GetFrameNum() - startFrame;
			std::chrono::duration<int64_t, std::nano> elapsedTime = std::chrono::system_clock::now() - startTime;
			double timeDurationSec = elapsedTime.count() / 1000000000.0;
			dev::Log("elapsed cpu cycles: {}, elapsed frames: {}, elapsed seconds: {}", elapsedCC, elapsedFrames, timeDurationSec);
		}

		while (m_status == Status::STOP)
		{
			ReqHandling(true);
		}
	}
}

// called from the external thread
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
				});
			break;

		case Req::GET_MEMORY_MAPPING:
			m_reqRes.emplace({
				{"mapping0", m_memory.GetState().mapping0.data},
				{"mapping1", m_memory.GetState().mapping1.data},
				});
			break;
		case Req::GET_GLOBAL_ADDR_RAM:
			m_reqRes.emplace({
				{"data", m_memory.GetGlobalAddrCheck(dataJ["addr"], Memory::AddrSpace::RAM)}
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
		case Req::IS_ROM_ENABLED:
			m_reqRes.emplace({
				{"data", m_memory.IsRomEnabled() },
				});
			break;             

		case Req::KEY_HANDLING:
		{
			auto op = m_io.GetKeyboard().KeyHandling(dataJ["key"], dataJ["action"]);
			if (op == Keyboard::Operation::RESET) {
				Reset();
			}
			else if (op == Keyboard::Operation::RESTART) {
				Restart();
			}
			m_reqRes.emplace({});
		}
			break;

		case Req::SCROLL_VERT:
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

		default:
			break;
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
	const auto cpuState = m_cpu.GetState();

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

auto dev::Hardware::GetByte(const nlohmann::json _addr, const Memory::AddrSpace _addrSpace)
-> nlohmann::json
{
	Addr addr = _addr["addr"];
	nlohmann::json out = {
		{"data", m_memory.GetByte(addr, _addrSpace)}
	};
	return out;
}

auto dev::Hardware::Get3Bytes(const nlohmann::json _addr, const Memory::AddrSpace _addrSpace)
-> nlohmann::json
{
	Addr addr = _addr["addr"];
	auto data = m_memory.GetByte(addr, _addrSpace) |
		m_memory.GetByte(addr + 1, _addrSpace) << 8 |
		m_memory.GetByte(addr + 2, _addrSpace) << 16;

	nlohmann::json out = {
		{"data", data }
	};
	return out;
}

auto dev::Hardware::GetWord(const nlohmann::json _addr, const Memory::AddrSpace _addrSpace)
-> nlohmann::json
{
	Addr addr = _addr["addr"];
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

// called from the external thread
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
	auto opcode = m_memory.GetByte(pc);
	auto cmdLen = dev::GetCmdLen(opcode);
	return pc + cmdLen;
}