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
	*((uint64_t*) &m_mappings[0].data) = 0;
	m_state.update.mapping.data = m_state.update.ramdiskIdx = m_mappingsEnabled = 0;
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

	return m_state.update.memType == MemType::ROM && globalAddr < m_rom.size() ?
		m_rom[globalAddr] : m_ram[globalAddr];
}

auto dev::Memory::CpuReadInstr(const Addr _addr, const AddrSpace _addrSpace,
	const uint8_t _byteNum)
	-> uint8_t
{
	auto globalAddr = GetGlobalAddr(_addr, _addrSpace);
	uint8_t val = m_state.update.memType == MemType::ROM && globalAddr < m_rom.size() ?
		m_rom[globalAddr] : m_ram[globalAddr];

	m_state.debug.instrGlobalAddr = _byteNum == 0 ? globalAddr : m_state.debug.instrGlobalAddr;
	m_state.debug.instr[_byteNum] = val;

	return val;
}

auto dev::Memory::CpuRead(const Addr _addr, const AddrSpace _addrSpace,
	const uint8_t _byteNum)
-> uint8_t
{
	auto globalAddr = GetGlobalAddr(_addr, _addrSpace);

	// debug
	m_state.debug.readGlobalAddr[_byteNum] = globalAddr;
	m_state.debug.readLen = _byteNum + 1;

	// return byte
	return m_state.update.memType == MemType::ROM && globalAddr < m_rom.size() ?
		m_rom[globalAddr] : m_ram[globalAddr];
}

// accessed by the CPU
// byteNum = 0 for the first byte stored by instr, 1 for the second
// _byteNum is 0 or 1
void dev::Memory::CpuWrite(const Addr _addr, uint8_t _value,
	const AddrSpace _addrSpace, const uint8_t _byteNum)
{
	auto globalAddr = GetGlobalAddr(_addr, _addrSpace);

	// debug
	m_state.update.beforeWrite[_byteNum] = m_ram[globalAddr];
	m_state.debug.writeGlobalAddr[_byteNum] = globalAddr;
	m_state.debug.writeLen = _byteNum + 1;

	m_state.debug.write[_byteNum] = _value;

	// store byte
	m_ram[globalAddr] = _value;
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
auto dev::Memory::GetGlobalAddr(const Addr _addr, const AddrSpace _addrSpace) const
-> GlobalAddr
{
	// if no mapping enabled, return _addr
	if ((!m_state.update.mapping.data & MAPPING_MODE_MASK)) return _addr;

	// check the STACK mapping
	if (m_state.update.mapping.modeStack && _addrSpace == AddrSpace::STACK)
	{
		return _addr + (m_state.update.mapping.pageStack + 1 + m_state.update.ramdiskIdx * 4) * RAM_DISK_PAGE_LEN;
	}
	// the ram mapping can be applied to a stack operation as well if the addr falls into the ram-mapping range
	if ((m_state.update.mapping.modeRamA && _addr >= 0xA000 && _addr < 0xE000) ||
		(m_state.update.mapping.modeRam8 && _addr >= 0x8000 && _addr < 0xA000) ||
		(m_state.update.mapping.modeRamE && _addr >= 0xE000))
	{
		return _addr + (m_state.update.mapping.pageRam + 1 + m_state.update.ramdiskIdx * 4) * RAM_DISK_PAGE_LEN;
	}

	return _addr;
}

// it raises an exception if the mapping is enabled for more than one Ram-disk.
// it used the first enabled Ram-disk during an exception
void dev::Memory::SetRamDiskMode(uint8_t _diskIdx, uint8_t _data) 
{	
	m_mappings[_diskIdx].data = _data;

	m_state.update.mapping.data = 0;
	m_mappingsEnabled = 0;

	for (int ramdiskIdx = 0; ramdiskIdx < RAMDISK_MAX; ramdiskIdx++)
	{
		if (m_mappings[ramdiskIdx].data & MAPPING_MODE_MASK)
		{
			if (++m_mappingsEnabled > 1) {
				break;
			}

			m_state.update.mapping.data = m_mappings[ramdiskIdx].data;
			m_state.update.ramdiskIdx = ramdiskIdx;
		}
	}
}

bool dev::Memory::IsException()
{
	auto out = m_mappingsEnabled > 1;
	m_mappingsEnabled = 0;
	return out;
}

bool dev::Memory::IsRomEnabled() const { return m_state.update.memType == MemType::ROM; };