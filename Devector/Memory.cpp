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
	std::copy(_data.begin(), _data.end(), m_data.data() + _loadAddr);
}

auto dev::Memory::GetByte(GlobalAddr _globalAddr, const AddrSpace _addrSpace)
-> uint8_t
{
	_globalAddr = GetGlobalAddr(_globalAddr, _addrSpace);
	return m_data[_globalAddr];
}

void dev::Memory::SetByte(GlobalAddr _globalAddr, uint8_t _value, const AddrSpace _addrSpace)
{
	_globalAddr = GetGlobalAddr(_globalAddr, _addrSpace);
	m_data[_globalAddr] = _value;
}

auto dev::Memory::GetWord(GlobalAddr _globalAddr, const AddrSpace _addrSpace)
-> uint16_t
{
	return GetByte(_globalAddr + 1, _addrSpace) << 8 | GetByte(_globalAddr, _addrSpace);
}

// reads 4 bytes from every screen buffer.
// all of these bytes are visually at the same position on the screen
auto dev::Memory::GetScreenSpaceBytes(Addr _screenSpaceAddrOffset) const
-> uint32_t
{
	auto byte8 = m_data[_screenSpaceAddrOffset + 0x8000];
	auto byteA = m_data[_screenSpaceAddrOffset + 0xA000];
	auto byteC = m_data[_screenSpaceAddrOffset + 0xC000];
	auto byteE = m_data[_screenSpaceAddrOffset + 0xE000];

	return byte8 << 24 | byteA << 16 | byteC << 8 | byteE;
}

auto dev::Memory::GetRam() const
-> const Ram*
{
	return &m_data;
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

void dev::Memory::SetRamDiskMode(uint8_t _data)
{
	// _data is encoded as E8AsSSMM
	//					E - the main ram mapped into 0xe000-0xffff, The Barkar Ram-Disk only
	//					8 - the main ram mapped into 0x8000-0x9fff, The Barkar Ram-Disk only
	//					A - the main ram mapped into 0xa000-0xdfff
	//					s - enabling a stack mapping
	//					SS - the Ram-Disk 64k page is used for stack operations mapping
	//					MM - the Ram-Disk 64k page is used for the ram operation mapping

	m_mappingModeStack = (_data & MAPPING_MODE_STACK_MASK) != 0;
	m_mappingModeRam = (_data & MAPPING_MODE_RAM_MASK) >> 5;

	m_mappingPageRam = _data & MAPPING_MODE_RAM_PAGE_MASK;
	m_mappingPageStack = (_data & MAPPING_MODE_STACK_PAGE_MASK) >> 2;
}
