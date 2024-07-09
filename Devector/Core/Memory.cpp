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

auto dev::Memory::GetByte(const Addr _addr, const AddrSpace _addrSpace)
-> uint8_t
{
	auto globalAddr = GetGlobalAddr(_addr, _addrSpace);

	if (m_state.update.memType == MemType::ROM && globalAddr < m_rom.size())
	{
		return m_rom[globalAddr];
	}

	return m_ram[globalAddr];
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

// converts the addr to a global addr depending on the ram/stack mapping modes
auto dev::Memory::GetGlobalAddr(Addr _addr, const AddrSpace _addrSpace) const
-> GlobalAddr
{
	// optimization. if mapping is off, return _addr
	if (m_state.mapping1.data + m_state.mapping2.data == 0) return _addr;

	// check the STACK mapping
	if (m_state.mapping1.modeStack && _addrSpace == AddrSpace::STACK)
	{
		return _addr + (m_state.mapping1.pageStack + 1) * RAM_DISK_PAGE_LEN;
	}
	// the ram mapping can be applied to a stack operation as well if the addr falls into the ram-mapping range
	return _addr + IsRamMapped(m_state.mapping1, _addr) * (m_state.mapping1.pageRam + 1) * RAM_DISK_PAGE_LEN;
}

auto dev::Memory::GetState() const -> const State& { return m_state; }
void dev::Memory::SetRamDiskMode(uint8_t _diskIdx, uint8_t _data)
{
	switch (_diskIdx) {
	case 0:
		m_state.mapping1.data = _data;
		break;
	case 1:
		m_state.mapping2.data = _data;
		break;
	}
}
bool dev::Memory::IsRomEnabled() const { return m_state.update.memType == MemType::ROM; };

// check if the addr is mapped to the ram-disk
auto dev::Memory::IsRamMapped(const Mapping _mapping, Addr _addr) const
-> GlobalAddr
{
	if ((_mapping.modeRamA && _addr >= 0xA000 && _addr <= 0xDFFF) ||
		(_mapping.modeRam8 && _addr >= 0x8000 && _addr <= 0x9FFF) ||
		(_mapping.modeRamE && _addr >= 0xE000))
	{
		return 1;
	}

	return 0;
}

