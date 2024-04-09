#include "Hardware.h"
#include "Utils/StringUtils.h"
#include <chrono>

dev::Hardware::Hardware()
    :
    m_status(Status::STOP),
    m_memory(),
    m_keyboard(),
    m_io(m_keyboard, m_memory, &Display::VectorColorToArgb),
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
    m_checkBreak.store(nullptr);
    m_cpu.AttachDebugOnReadInstr(nullptr);
    m_cpu.AttachDebugOnRead(nullptr);
    m_cpu.AttachDebugOnWrite(nullptr);

    Request(Hardware::Req::EXIT);
    m_executionThread.join();
}

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
                ExecuteInstruction();

                ReqHandling();

                auto CheckBreak = m_checkBreak.load();
                auto pcGlobalAddr = m_memory.GetGlobalAddr(m_cpu.GetPC(), Memory::RAM);

                if (CheckBreak && *CheckBreak && (*CheckBreak)(pcGlobalAddr)) 
                {
                    m_status = Status::STOP;
                }

            } while (m_status == Status::RUN && m_display.GetFrameNum() == frameNum);

            // vsync
            if (m_status == Status::RUN)
            {
                expectedTime = expectedTime + Display::VSYC_DELAY;

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

void dev::Hardware::AttachCheckBreak(CheckBreakFunc* _funcP) { m_checkBreak.store(_funcP); }
void dev::Hardware::AttachDebugOnReadInstr(I8080::DebugOnReadInstrFunc* _funcP) { m_cpu.AttachDebugOnReadInstr(_funcP); }
void dev::Hardware::AttachDebugOnRead(I8080::DebugOnReadFunc* _funcP) { m_cpu.AttachDebugOnRead(_funcP); }
void dev::Hardware::AttachDebugOnWrite(I8080::DebugOnWriteFunc* _funcP) { m_cpu.AttachDebugOnWrite(_funcP); }

// internal thread
void dev::Hardware::ReqHandling(const bool _waitReq)
{
    if (!m_reqs.empty() || _waitReq)
    {        
        auto result = m_reqs.pop();

        const auto& [req, dataj] = *result;

        switch (req)
        {
        case Req::RUN:
            m_status = Status::RUN;
            m_reqRes.emplace({});
            break;

        case Req::STOP:
            m_status = Status::STOP;
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

        case Req::SET_MEM:
            SetMem(dataj);
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
        case Req::GET_REGS:
            m_reqRes.emplace(GetRegs());
            break;

        case Req::GET_REG_PC:
            m_reqRes.emplace({
                {"pc", m_cpu.GetPC() },
                });
            break;

        case Req::GET_BYTE_RAM:
            m_reqRes.emplace(GetByte(dataj, Memory::AddrSpace::RAM));
            break;

        case Req::GET_WORD_STACK:
            m_reqRes.emplace(GetWord(dataj, Memory::AddrSpace::STACK));
            break;

        case Req::GET_DISPLAY_DATA:
            m_reqRes.emplace({
                {"rasterLine", m_display.GetRasterLine()},
                {"rasterPixel", m_display.GetRasterPixel()},
                });
            break;

        case Req::GET_MEMORY_MODES:
            m_reqRes.emplace({
                {"mappingModeStack", m_memory.m_mappingModeStack},
                {"mappingPageStack", m_memory.m_mappingPageStack},
                {"mappingModeRam", m_memory.m_mappingModeRam},
                {"mappingPageRam", m_memory.m_mappingPageRam}
                });
            break;

        case Req::GET_GLOBAL_ADDR_RAM:
            m_reqRes.emplace({
                {"data", m_memory.GetGlobalAddr(dataj["addr"], Memory::AddrSpace::RAM)}
                });
            break;

        case Req::KEY_HANDLING:
            m_io.GetKeyboard().KeyHandling(dataj["key"], dataj["action"]);
            m_reqRes.emplace({});
            break;

        case Req::SCROLL_VERT:
            m_reqRes.emplace({
                {"scrollVert", m_display.GetScrollVert()}
                });
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
}

void dev::Hardware::Restart()
{
    m_cpu.Reset();
}

void dev::Hardware::SetMem(const nlohmann::json& _dataJ)
{
    const std::vector<uint8_t> data = _dataJ;

    if (data.size() > Memory::MEMORY_MAIN_LEN) {
        // TODO: communicate the fail state
        return;
    }

    m_memory.Set(data);
}

auto dev::Hardware::GetRegs() const
-> nlohmann::json
{
    nlohmann::json out = {
        {"cc", m_cpu.GetCC() },
        {"pc", m_cpu.GetPC() },
        {"sp", m_cpu.GetSP() },
        {"af", m_cpu.GetAF() },
        {"bc", m_cpu.GetBC() },
        {"de", m_cpu.GetDE() },
        {"hl", m_cpu.GetHL() },
        {"flagS",   m_cpu.GetFlagS() },
        {"flagZ",   m_cpu.GetFlagZ() },
        {"flagAC",  m_cpu.GetFlagAC() },
        {"flagP",   m_cpu.GetFlagP() },
        {"flagC",   m_cpu.GetFlagC() },
        {"flagINTE",m_cpu.GetINTE() },
        {"flagIFF", m_cpu.GetIFF() },
        {"flagHLTA",m_cpu.GetHLTA() },
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

auto dev::Hardware::GetWord(const nlohmann::json _addr, const Memory::AddrSpace _addrSpace)
-> nlohmann::json
{
    Addr addr = _addr["addr"];
    nlohmann::json out = {
        {"data", m_memory.GetWord(addr, _addrSpace)}
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