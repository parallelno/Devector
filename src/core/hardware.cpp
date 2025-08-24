#include "core/hardware.h"
#include "utils/str_utils.h"
#include "core/disasm.h"

dev::Hardware::Hardware(const std::string& _pathBootData,
		const std::string& _pathRamDiskData, const bool _ramDiskClearAfterRestart)
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
		m_memory.InitRamDiskMapping(); // reset RAM Disk mode collision
		dev::Log("ERROR: more than one RAM Disk has mapping enabled");
		return true;
	}

	ReqHandling(0ns);

	return false;
}

// TODO:
// 1. reload, reset, update the palette, and other non-hardware-initiated operations have to reset the playback history
// 2. navigation. show data as data in the disasm. take the list from the watchpoints
// 3. aggregation of consts, labels, funcs with default names

void dev::Hardware::Execution()
{
	while (m_status != Status::EXIT)
	{
		auto startCC = m_cpu.GetCC();
		auto startFrame = m_display.GetFrameNum();
		auto startTime = std::chrono::system_clock::now();

		auto endFrameTime = std::chrono::system_clock::now();

		while (m_status == Status::RUN)
		{
			auto startFrameTime = std::chrono::system_clock::now();

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
				auto currentTime = std::chrono::system_clock::now();
    			auto frameExecutionDuration = currentTime - startFrameTime;

				auto targetFrameDuration = m_execDelays[static_cast<size_t>(m_execSpeed)];

				using DurationType = decltype(frameExecutionDuration);
				auto frameDuration = std::max<DurationType>(
					frameExecutionDuration + m_reqHandlingTime,
					targetFrameDuration
				);

				endFrameTime += frameDuration;

				while (std::chrono::system_clock::now() < endFrameTime)
				{
					// Use smaller time slice for request handling
					ReqHandling(m_reqHandlingTime / 10);
				}
			}
		}

		// print out the break statistics
		auto elapsedCC = m_cpu.GetCC() - startCC;
		if (elapsedCC) {
			auto elapsedFrames = m_display.GetFrameNum() - startFrame;
			auto elapsedTime = (std::chrono::duration<int64_t, std::nano>)(std::chrono::system_clock::now() - startTime);
			double timeDurationSec = elapsedTime.count() / 1000000000.0;
			dev::Log("Break: elapsed cpu cycles: {}, elapsed frames: {}, elapsed seconds: {}", elapsedCC, elapsedFrames, timeDurationSec);
		}

		while (m_status == Status::STOP)
		{
			ReqHandling();
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
void dev::Hardware::ReqHandling(const std::chrono::duration<int64_t, std::nano> _waitTime)
{
	auto result = m_reqs.pop(_waitTime);
	if (!result) return;

	const auto& [req, dataJ] = *result;

	nlohmann::json out;

	switch (req)
	{
	case Req::RUN:
		Run();
		break;

	case Req::STOP:
		Stop();
		break;

	case Req::IS_RUNNING:
		out = {
			{"isRunning", m_status == Status::RUN},
			};
		break;

	case Req::EXIT:
		m_status = Status::EXIT;
		break;

	case Req::RESET:
		Reset();
		break;

	case Req::RESTART:
		Restart();
		break;

	case Req::EXECUTE_INSTR:
		ExecuteInstruction();
		break;

	case Req::EXECUTE_FRAME_NO_BREAKS:
	{
		ExecuteFrameNoBreaks();
		break;
	}
	case Req::GET_CC:
		out = {
			{"cc", m_cpu.GetCC() },
			};
		break;

	case Req::GET_REGS:
		out = GetRegs();
		break;

	case Req::GET_REG_PC:
		out = {
			{"pc", m_cpu.GetPC() },
			};
		break;

	case Req::GET_RUSLAT_HISTORY:
		out = {
			{"data", m_io.GetRusLatHistory()},
			};
		break;

	case Req::GET_IO_PALETTE:
	{
		auto data = m_io.GetPalette();
		out = {
			{"low", data->low},
			{"hi", data->hi},
			};
		break;
	}
	case Req::GET_IO_PORTS:
	{
		auto data = m_io.GetPorts();
		out = {
			{"data", data->data},
			};
		break;
	}

	case Req::GET_IO_PALETTE_COMMIT_TIME:
	{
		auto data = m_io.GetPaletteCommitTime();
		out = {
			{"paletteCommitTime", data},
			};
		break;
	}

	case Req::SET_IO_PALETTE_COMMIT_TIME:
	{
		m_io.SetPaletteCommitTime(dataJ["paletteCommitTime"]);
		break;
	}

	case Req::GET_DISPLAY_BORDER_LEFT:
	{
		auto data = m_display.GetBorderLeft();
		out = {
			{"borderLeft", data},
			};
		break;
	}

	case Req::SET_DISPLAY_BORDER_LEFT:
	{
		m_display.SetBorderLeft(dataJ["borderLeft"]);
		break;
	}

	case Req::GET_DISPLAY_IRQ_COMMIT_PXL:
	{
		auto data = m_display.GetIrqCommitPxl();
		out = {
			{"irqCommitPxl", data},
			};
		break;
	}

	case Req::SET_DISPLAY_IRQ_COMMIT_PXL:
	{
		m_display.SetIrqCommitPxl(dataJ["irqCommitPxl"]);
		break;
	}

	case Req::GET_IO_DISPLAY_MODE:
		out = {
			{"data", m_io.GetDisplayMode()},
			};
		break;

	case Req::GET_BYTE_GLOBAL:
		out = GetByteGlobal(dataJ);
		break;

	case Req::GET_BYTE_RAM:
		out = GetByte(dataJ, Memory::AddrSpace::RAM);
		break;

	case Req::GET_THREE_BYTES_RAM:
		out = Get3Bytes(dataJ, Memory::AddrSpace::RAM);
		break;

	case Req::GET_MEM_STRING_GLOBAL:
		out = GetMemString(dataJ);
		break;

	case Req::GET_WORD_STACK:
		out = GetWord(dataJ, Memory::AddrSpace::STACK);
		break;

	case Req::GET_STACK_SAMPLE:
		out = GetStackSample(dataJ);
		break;

	case Req::GET_DISPLAY_DATA:
		out = {
			{"rasterLine", m_display.GetRasterLine()},
			{"rasterPixel", m_display.GetRasterPixel()},
			{"frameNum", m_display.GetFrameNum()},
			};
		break;

	case Req::GET_MEMORY_MAPPING:
		out = {
			{"mapping", m_memory.GetState().update.mapping.data},
			{"ramdiskIdx", m_memory.GetState().update.ramdiskIdx},
			};
		break;

	case Req::GET_MEMORY_MAPPINGS:{
		auto mappingsP = m_memory.GetMappingsP();
		out = {{"ramdiskIdx", m_memory.GetState().update.ramdiskIdx}};
		for (auto i=0; i < Memory::RAM_DISK_MAX; i++) {
			out["mapping"+std::to_string(i)] = mappingsP[i].data;
		}
		break;
	}
	case Req::GET_GLOBAL_ADDR_RAM:
		out = {
			{"data", m_memory.GetGlobalAddr(dataJ["addr"], Memory::AddrSpace::RAM)}
			};
		break;

	case Req::GET_FDC_INFO: {
		auto info = m_fdc.GetFdcInfo();
		out = {
			{"drive", info.drive},
			{"side", info.side},
			{"track", info.track},
			{"lastS", info.lastS},
			{"wait", info.irq},
			{"cmd", info.cmd},
			{"rwLen", info.rwLen},
			{"position", info.position},
			};
		break;
	}

	case Req::GET_FDD_INFO: {
		auto info = m_fdc.GetFddInfo(dataJ["driveIdx"]);
		out = {
			{"path", info.path},
			{"updated", info.updated},
			{"reads", info.reads},
			{"writes", info.writes},
			{"mounted", info.mounted},
			};
		break;
	}

	case Req::GET_FDD_IMAGE:
		out = {
			{"data", m_fdc.GetFddImage(dataJ["driveIdx"])},
			};
		break;

	case Req::GET_STEP_OVER_ADDR:
		out = {
			{"data", GetStepOverAddr()},
			};
		break;

	case Req::GET_IO_PORTS_IN_DATA:
	{
		auto portsData = m_io.GetPortsInData();
		out = {
			{"data0", portsData->data0},
			{"data1", portsData->data1},
			{"data2", portsData->data2},
			{"data3", portsData->data3},
			{"data4", portsData->data4},
			{"data5", portsData->data5},
			{"data6", portsData->data6},
			{"data7", portsData->data7},
			};
		break;
	}
	case Req::GET_IO_PORTS_OUT_DATA:
	{
		auto portsData = m_io.GetPortsOutData();
		out = {
			{"data0", portsData->data0},
			{"data1", portsData->data1},
			{"data2", portsData->data2},
			{"data3", portsData->data3},
			{"data4", portsData->data4},
			{"data5", portsData->data5},
			{"data6", portsData->data6},
			{"data7", portsData->data7},
			};
		break;
	}
	case Req::SET_MEM:
		m_memory.SetRam(dataJ["addr"], dataJ["data"]);
		break;

	case Req::SET_BYTE_GLOBAL:
		m_memory.SetByteGlobal(dataJ["addr"], dataJ["data"]);
		break;

	case Req::SET_CPU_SPEED:
	{
		int speed = dataJ["speed"];
		speed = std::clamp(speed, 0, int(sizeof(m_execDelays) - 1));
		m_execSpeed = static_cast<ExecSpeed>(speed);
		if (m_execSpeed == ExecSpeed::_20PERCENT) { m_audio.Mute(true); }
		else { m_audio.Mute(false); }
		break;
	}

	case Req::GET_HW_MAIN_STATS:
	{
		auto paletteP = m_io.GetPalette();

		out = {{"cc", m_cpu.GetCC()},
			{"rasterLine", m_display.GetRasterLine()},
			{"rasterPixel", m_display.GetRasterPixel()},
			{"frameCc", (m_display.GetRasterPixel() + m_display.GetRasterLine() * Display::FRAME_W) / 4},
			{"frameNum", m_display.GetFrameNum()},
			{"displayMode", m_io.GetDisplayMode()},
			{"scrollVert", m_display.GetScrollVert()},
			{"rusLat", (m_io.GetRusLatHistory() & 0b1000) != 0},
			{"inte", m_cpu.GetState().ints.inte},
			{"iff", m_cpu.GetState().ints.iff},
			{"hlta", m_cpu.GetState().ints.hlta},
			};
			for (int i=0; i < IO::PALETTE_LEN; i++ ){
				out["palette"+std::to_string(i)] = Display::VectorColorToArgb(paletteP->bytes[i]);
			}
		break;
	}
	case Req::IS_MEMROM_ENABLED:
		out = {
			{"data", m_memory.IsRomEnabled() },
			};
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
		break;
	}
	case Req::GET_SCROLL_VERT:
		out = {
			{"scrollVert", m_display.GetScrollVert()}
			};
		break;

	case Req::LOAD_FDD:
		m_fdc.Mount(dataJ["driveIdx"], dataJ["data"], dataJ["path"]);
		break;

	case Req::RESET_UPDATE_FDD:
		m_fdc.ResetUpdate(dataJ["driveIdx"]);
		break;

	case Req::DEBUG_ATTACH:
		m_debugAttached = dataJ["data"];
		break;

	default:
		out = DebugReqHandling(req, dataJ, m_cpu.GetStateP(), m_memory.GetStateP(), m_io.GetStateP(), m_display.GetStateP());
	}

	m_reqRes.emplace(std::move(out));
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
		{"m", m_memory.GetByte(cpuState.regs.hl.word, Memory::AddrSpace::RAM)}
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

static constexpr int BYTES_IN_LINE = 16;
static constexpr int CHARS_IN_LINE = BYTES_IN_LINE; // 1 char per byte
static constexpr int HEX_LEN = 3;
static constexpr int HEX_CHARS_IN_LINE = BYTES_IN_LINE * HEX_LEN; // FF and space
static constexpr int SPACE_LEN = 1;
static constexpr int NEWLINE_LEN = 1;
static constexpr int EOF_LEN = 1;
static constexpr int LINES_MAX = 16;
static const int LINE_LEN_MAX = HEX_CHARS_IN_LINE + 1 + CHARS_IN_LINE + NEWLINE_LEN;

static char hex_data[LINE_LEN_MAX * LINES_MAX + EOF_LEN] = { 0 };

static bool init_hex_data() {
	for (int i = 0; i < sizeof(hex_data)-1; i++)
	{
		int x = i % LINE_LEN_MAX;
		int y = i / LINE_LEN_MAX;

		// end of line
		hex_data[i] = x == LINE_LEN_MAX - 1 ? '\n' : ' ';
	}
	return true;
}

static bool hex_data_initialized = init_hex_data();

auto dev::Hardware::GetMemString(const nlohmann::json _dataJ)
-> nlohmann::json
{
	GlobalAddr globalAddr = _dataJ["addr"];
	Addr len = _dataJ["len"];
	len = len > 255 ? 255 : len;
	int char_idx = 0;
	int line_len = len < BYTES_IN_LINE ? len : BYTES_IN_LINE;

	for (Addr addrOffset = 0; addrOffset < len; addrOffset++)
	{
		auto c = m_memory.GetByteGlobal(globalAddr + addrOffset);
		int x = addrOffset % BYTES_IN_LINE;
		int y = addrOffset / BYTES_IN_LINE;

		// hex
		int hex_idx = LINE_LEN_MAX * y + x * HEX_LEN;
		uint8_t l = c & 0x0F;
		uint8_t h = (c >> 4) & 0x0F;
		hex_data[hex_idx] = h < 10 ? ('0' + h) : ('A' + h - 10);
		hex_data[hex_idx + 1] = l < 10 ? ('0' + l) : ('A' + l - 10);
		hex_data[hex_idx + 2] = ' ';

		if (x == 0)
		{
			// a break between hex and chars
			hex_data[LINE_LEN_MAX * y + HEX_LEN * line_len] = ' ';
		}

		// char
		char_idx = LINE_LEN_MAX * y + HEX_LEN * line_len + SPACE_LEN + x;
		hex_data[char_idx] = c > 31 && c < 127 ? (char)c : '.';

		// newline
		if (x == LINE_LEN_MAX - 1){
			int newline_idx = LINE_LEN_MAX * y + HEX_CHARS_IN_LINE + 1 + CHARS_IN_LINE;
			hex_data[newline_idx] = '\n';
		}
	}

	// end of file
	hex_data[char_idx + 1] = '\0';

	nlohmann::json out = {
		{"data", hex_data },
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

auto dev::Hardware::GetStackSample(const nlohmann::json _addrJ)
-> nlohmann::json
{
	Addr addr = _addrJ["addr"];
	auto dataN10 = m_memory.GetByte(addr - 9, Memory::AddrSpace::STACK) << 8 | m_memory.GetByte(addr - 10, Memory::AddrSpace::STACK);
	auto dataN8 = m_memory.GetByte(addr - 7, Memory::AddrSpace::STACK) << 8 | m_memory.GetByte(addr - 8, Memory::AddrSpace::STACK);
	auto dataN6 = m_memory.GetByte(addr - 5, Memory::AddrSpace::STACK) << 8 | m_memory.GetByte(addr - 6, Memory::AddrSpace::STACK);
	auto dataN4 = m_memory.GetByte(addr - 3, Memory::AddrSpace::STACK) << 8 | m_memory.GetByte(addr - 4, Memory::AddrSpace::STACK);
	auto dataN2 = m_memory.GetByte(addr - 1, Memory::AddrSpace::STACK) << 8 | m_memory.GetByte(addr - 2, Memory::AddrSpace::STACK);
	auto data = m_memory.GetByte(addr + 1, Memory::AddrSpace::STACK) << 8 | m_memory.GetByte(addr, Memory::AddrSpace::STACK);
	auto dataP2 = m_memory.GetByte(addr + 3, Memory::AddrSpace::STACK) << 8 | m_memory.GetByte(addr + 2, Memory::AddrSpace::STACK);
	auto dataP4 = m_memory.GetByte(addr + 5, Memory::AddrSpace::STACK) << 8 | m_memory.GetByte(addr + 4, Memory::AddrSpace::STACK);
	auto dataP6 = m_memory.GetByte(addr + 7, Memory::AddrSpace::STACK) << 8 | m_memory.GetByte(addr + 6, Memory::AddrSpace::STACK);
	auto dataP8 = m_memory.GetByte(addr + 9, Memory::AddrSpace::STACK) << 8 | m_memory.GetByte(addr + 8, Memory::AddrSpace::STACK);
	auto dataP10 = m_memory.GetByte(addr + 11, Memory::AddrSpace::STACK) << 8 | m_memory.GetByte(addr + 10, Memory::AddrSpace::STACK);

	nlohmann::json out = {
		{"-10", dataN10},
		{"-8", dataN8},
		{"-6", dataN6},
		{"-4", dataN4},
		{"-2", dataN2},
		{"0", data},
		{"2", dataP2},
		{"4", dataP4},
		{"6", dataP6},
		{"8", dataP8},
		{"10", dataP10},
	};
	return out;
}

auto dev::Hardware::GetRam() const
-> const Memory::Ram*
{
	return m_memory.GetRam();
}

// UI thread. Non-blocking reading.
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

	auto im_addr = m_memory.GetByte(pc + 2) << 8 | m_memory.GetByte(pc + 1);
	auto next_pc = pc + dev::GetCmdLen(opcode);

	switch (dev::GetOpcodeType(opcode))
	{
	case OPTYPE_JMP:
		return im_addr;
	case OPTYPE_RET:
		return m_memory.GetByte(sp + 1, Memory::AddrSpace::STACK) << 8 | m_memory.GetByte(sp, Memory::AddrSpace::STACK);
	case OPTYPE_PCH:
		return m_cpu.GetHL();
	case OPTYPE_RST:
		return opcode - CpuI8080::OPCODE_RST0;
	default:
		switch (opcode)
		{
		case CpuI8080::OPCODE_JNZ:
			return m_cpu.GetFlagZ() ? next_pc : im_addr;
		case CpuI8080::OPCODE_JZ:
			return m_cpu.GetFlagZ() ? im_addr : next_pc;
		case CpuI8080::OPCODE_JNC:
			return m_cpu.GetFlagC() ? next_pc : im_addr;
		case CpuI8080::OPCODE_JC:
			return m_cpu.GetFlagC() ? im_addr : next_pc;
		case CpuI8080::OPCODE_JPO:
			return m_cpu.GetFlagP() ? next_pc : im_addr;
		case CpuI8080::OPCODE_JPE:
			return m_cpu.GetFlagP() ? im_addr : next_pc;
		case CpuI8080::OPCODE_JP:
			return m_cpu.GetFlagS() ? next_pc : im_addr;
		case CpuI8080::OPCODE_JM:
			return m_cpu.GetFlagS() ? im_addr : next_pc;
		default:
			break;
		}
	}
	return next_pc;
}