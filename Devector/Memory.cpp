#include "Memory.h"

void dev::Memory::Init()
{
    std::fill(m_data, m_data + std::size(m_data), 0);

    m_mappingModeStack = false;
    m_mappingPageStack = 0;
    m_mappingModeRam = 0;
    m_mappingPageRam = 0;
}

void dev::Memory::Load(const std::vector<uint8_t>& _data)
{
    std::copy(_data.begin(), _data.end(), m_data);
}

auto dev::Memory::GetByte(uint32_t _addr, AddrSpace _addr_space) const
-> uint8_t
{
    return 0;
}

void dev::Memory::SetByte(uint32_t _addr, uint8_t _value, AddrSpace _addr_space)
{
}

int dev::Memory::GetWord(uint32_t _addr, AddrSpace _addrSpace) const
{
    auto addr0 = GetGlobalAddr(_addr, _addrSpace);
    auto addr1 = GetGlobalAddr(_addr + 1, _addrSpace);
    auto lb = m_data[addr0];
    auto hb = m_data[addr1];
    return hb << 8 | lb;
}

int dev::Memory::Length()
{
    return GLOBAL_MEMORY_LEN;
}

// converts an UInt16 addr to a global addr depending on the ram/stack mapping modes
uint32_t dev::Memory::GetGlobalAddr(uint32_t _addr, AddrSpace _addrSpace) const
{
    if (_addrSpace == AddrSpace::GLOBAL) return _addr % GLOBAL_MEMORY_LEN;

    _addr &= 0xffff;

    if (_addrSpace == AddrSpace::STACK)
    {
        if (m_mappingModeStack)
        {
            return _addr + m_mappingPageStack * RAM_DISK_PAGE_LEN;
        }
    }
    else if (_addrSpace == AddrSpace::RAM)
    {
        if (_addr < 0x8000 || m_mappingModeRam == 0) return _addr;

        if (((m_mappingModeRam & 0x20) > 0) && (_addr >= 0xa000) && (_addr <= 0xdfff))
        {
            return _addr + m_mappingPageRam * RAM_DISK_PAGE_LEN;
        }
        if (((m_mappingModeRam & 0x40) > 0) && (_addr >= 0x8000) && (_addr <= 0x9fff))
        {
            return _addr + m_mappingPageRam * RAM_DISK_PAGE_LEN;
        }
        if (((m_mappingModeRam & 0x80) > 0) && (_addr >= 0xe000) && (_addr <= 0xffff))
        {
            return _addr + m_mappingPageRam * RAM_DISK_PAGE_LEN;
        }
    }

    return _addr;
}
