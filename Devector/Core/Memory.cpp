#include "Memory.h"
#include "Utils/Utils.h"

dev::Memory::Memory(const std::wstring& _pathBootData)
	: m_rom(), m_ram()
{
	auto res = dev::LoadFile(_pathBootData);
	if (res) m_rom = *res;
}
void dev::Memory::Init()
{
	m_ram.fill(0);
	m_state.mapping1.data = 0;
	m_state.update.memType = MemType::ROM;
}

void dev::Memory::Restart() { m_state.update.memType = MemType::RAM; }


void dev::Memory::SetMemType(const MemType _memType)
{
	m_state.update.memType = _memType;
}
void dev::Memory::SetRam(const Addr _addr, const std::vector<uint8_t>& _data )
{
	std::copy(_data.begin(), _data.end(), m_ram.data() + _addr);
}

auto dev::Memory::GetByte(GlobalAddr _globalAddr, const AddrSpace _addrSpace)
-> uint8_t
{
	_globalAddr = GetGlobalAddr(_globalAddr, _addrSpace);

	if (m_state.update.memType == MemType::ROM && _globalAddr < m_rom.size())
	{
		return m_rom[_globalAddr];
	}

	return m_ram[_globalAddr];
}

// byteNum = 0 for the first byte stored by instr, 1 for the second
void dev::Memory::SetByte(const Addr _addr, uint8_t _value, 
	const AddrSpace _addrSpace, const uint8_t _byteNum)
{
	auto globalAddr = GetGlobalAddr(_addr, _addrSpace);
	m_ram[globalAddr] = _value;
	m_state.update.addr = _addr;
	m_state.update.len = _byteNum;
	m_state.update.stack = static_cast<uint8_t>(_addrSpace);
	m_state.update.b1 = _value * (1 - _byteNum) + m_state.update.b1 * _byteNum;
	m_state.update.b2 = _value;
}

// help func. isn't called by cpu, not stored to the state
auto dev::Memory::GetWord(GlobalAddr _globalAddr, const AddrSpace _addrSpace)
-> uint16_t
{
	return GetByte(_globalAddr + 1, _addrSpace) << 8 | GetByte(_globalAddr, _addrSpace);
}

// reads 4 bytes from every screen buffer.
// all of these bytes are visually at the same position on the screen
auto dev::Memory::GetScreenBytes(Addr _screenAddrOffset) const
-> uint32_t
{
	auto byte8 = m_ram[_screenAddrOffset + 0x8000];
	auto byteA = m_ram[_screenAddrOffset + 0xA000];
	auto byteC = m_ram[_screenAddrOffset + 0xC000];
	auto byteE = m_ram[_screenAddrOffset + 0xE000];

	return byte8 << 24 | byteA << 16 | byteC << 8 | byteE;
}

auto dev::Memory::GetRam() const -> const Ram* { return &m_ram; }

// converts an addr to a global addr depending on the ram/stack mapping modes
auto dev::Memory::GetGlobalAddr(GlobalAddr _globalAddr, const AddrSpace _addrSpace) const
-> GlobalAddr
{
	if (_addrSpace == AddrSpace::GLOBAL) return _globalAddr % GLOBAL_MEMORY_LEN;

	_globalAddr &= 0xFFFF;

	if (m_state.mapping1.modeStack && _addrSpace == AddrSpace::STACK)
	{
		return _globalAddr + static_cast<GlobalAddr>(m_state.mapping1.pageStack + 1) * RAM_DISK_PAGE_LEN;
	}
	else if (IsRamMapped((Addr)_globalAddr))
	{
		return _globalAddr + (m_state.mapping1.pageRam + 1) * RAM_DISK_PAGE_LEN;
	}

	return _globalAddr;
}

auto dev::Memory::GetState() const -> const State& { return m_state; }
void dev::Memory::SetRamDiskMode(uint8_t _data) { m_state.mapping1.data = _data; }
bool dev::Memory::IsRomEnabled() const { return m_state.update.memType == MemType::ROM; };

// check if the addr is mapped to the ram-disk
bool dev::Memory::IsRamMapped(Addr _addr) const
{
	if ((m_state.mapping1.modeRamA && _addr >= 0xA000 && _addr <= 0xDFFF) ||
		(m_state.mapping1.modeRam8 && _addr >= 0x8000 && _addr <= 0x9FFF) ||
		(m_state.mapping1.modeRamE && _addr >= 0xE000 && _addr <= 0xFFFF))
	{
		return true;
	}

	return false;
}

