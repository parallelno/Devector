#include "Hardware.h"

dev::Hardware::Hardware()
    :
    m_memory(),
    m_io(),
    m_debugger(),
    m_cpu(
        std::bind(&Memory::GetByte, &m_memory, std::placeholders::_2),
        std::bind(&Memory::SetByte, &m_memory, std::placeholders::_3),
        std::bind(&IO::PortIn, &m_io, std::placeholders::_1),
        std::bind(&IO::PortOut, &m_io, std::placeholders::_2),
        std::bind(&Debugger::MemStats, &m_debugger, std::placeholders::_3)),
    m_display(m_memory)
{}

void dev::Hardware::LoadRom(std::string path)
{
}

void dev::Hardware::ExecuteFrame()
{
}

void dev::Hardware::ExecuteInstruction()
{
}

void dev::Hardware::Init()
{
}
