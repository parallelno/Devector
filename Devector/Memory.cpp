#include "Memory.h"

auto dev::Memory::GetByte(uint32_t _addr, Memory::AddrSpace _addr_space) -> uint8_t
{
    return 0;
}

void dev::Memory::SetByte(uint32_t _addr, uint8_t _value, Memory::AddrSpace _addr_space)
{
}

void dev::Memory::Init()
{
    std::fill(m_memory, m_memory + std::size(m_memory), 0);

    m_mappingModeStack = false;
    m_mappingPageStack = 0;
    m_mappingModeRam = 0;
    m_mappingPageRam = 0;
}

void dev::Memory::Load(const std::vector<uint8_t>& _data)
{
    std::copy(_data.begin(), _data.end(), m_memory);
}
