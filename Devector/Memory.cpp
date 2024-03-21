#include "Memory.h"

void dev::Memory::Init()
{
    m_data.fill(0);

    m_mappingModeStack = false;
    m_mappingPageStack = 0;
    m_mappingModeRam = 0;
    m_mappingPageRam = 0;
}

void dev::Memory::Set(const std::vector<uint8_t>& _data, const Addr _loadAddr)
{
    std::unique_lock<std::mutex> mlock(m_ramMutex);
    std::copy(_data.begin(), _data.end(), m_data.data() + _loadAddr);
}

auto dev::Memory::GetByte(GlobalAddr _globalAddr, const AddrSpace _addrSpace)
-> uint8_t
{
    _globalAddr = GetGlobalAddr(_globalAddr, _addrSpace);
    std::unique_lock<std::mutex> mlock(m_ramMutex);
    return m_data[_globalAddr];
}

void dev::Memory::SetByte(GlobalAddr _globalAddr, uint8_t _value, const AddrSpace _addrSpace)
{
    _globalAddr = GetGlobalAddr(_globalAddr, _addrSpace);
    std::unique_lock<std::mutex> mlock(m_ramMutex);
    m_data[_globalAddr] = _value;
}

auto dev::Memory::GetWord(GlobalAddr _globalAddr, const AddrSpace _addrSpace)
-> uint16_t
{
    return GetByte(_globalAddr + 1, _addrSpace) << 8 | GetByte(_globalAddr, _addrSpace);
}

auto dev::Memory::GetRam8K(const Addr _addr)
-> const OutRam*
{
    std::unique_lock<std::mutex> mlock(m_ramMutex);
    std::copy(m_data.begin(), m_data.begin() + m_out.size(), m_out.begin());

    return &m_out;
}

// converts an addr to a global addr depending on the ram/stack mapping modes
auto dev::Memory::GetGlobalAddr(const GlobalAddr _globalAddr, const AddrSpace _addrSpace) const
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

