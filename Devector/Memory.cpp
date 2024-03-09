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
    std::copy(_data.begin(), _data.end(), m_data + ROM_LOAD_ADDR);
}

auto dev::Memory::GetByte(GlobalAddr _globalAddr, AddrSpace _addrSpace) const
-> uint8_t
{
    _globalAddr = GetGlobalAddr(_globalAddr, _addrSpace);
    return m_data[_globalAddr];
}

void dev::Memory::SetByte(GlobalAddr _globalAddr, uint8_t _value, AddrSpace _addrSpace)
{
    _globalAddr = GetGlobalAddr(_globalAddr, _addrSpace);
    m_data[_globalAddr] = _value;
}

auto dev::Memory::GetWord(GlobalAddr _globalAddr, AddrSpace _addrSpace) const
-> uint16_t
{
    return GetByte(_globalAddr + 1, _addrSpace) << 8 | GetByte(_globalAddr, _addrSpace);
}

int dev::Memory::Length()
{
    return GLOBAL_MEMORY_LEN;
}

// converts an addr to a global addr depending on the ram/stack mapping modes
auto dev::Memory::GetGlobalAddr(const GlobalAddr _globalAddr, AddrSpace _addrSpace) const
-> GlobalAddr
{
    if (_addrSpace == AddrSpace::GLOBAL) return _globalAddr % GLOBAL_MEMORY_LEN;

    Addr addr = _globalAddr & 0xFFFF;

    if (_addrSpace == AddrSpace::STACK)
    {
        if (m_mappingModeStack)
        {
            return (GlobalAddr)(addr + (m_mappingPageStack + 1) * RAM_DISK_PAGE_LEN);
        }
    }
    else if (_addrSpace == AddrSpace::RAM && IsRamMapped(addr))
    {
        return (GlobalAddr)(addr + (m_mappingPageRam + 1) * RAM_DISK_PAGE_LEN);
    }

    return addr;
}

// check if the addr is mapped to the ram-disk
bool dev::Memory::IsRamMapped(Addr _addr) const
{
    if ((m_mappingModeRam & MAPPING_MODE_RAM_A000) && (_addr >= 0xa000) && (_addr <= 0xdfff) ||
        (m_mappingModeRam & MAPPING_MODE_RAM_8000) && (_addr >= 0x8000) && (_addr <= 0x9fff) ||
        (m_mappingModeRam & MAPPING_MODE_RAM_E000) && (_addr >= 0xe000) && (_addr <= 0xffff))
    {
        return true;
    }

    return false;
}
