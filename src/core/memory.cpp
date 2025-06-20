#include "core/memory.h"
#include "utils/utils.h"

dev::Memory::Memory(const std::string& _pathBootData, const std::string& _pathRamDiskData, 
	const bool _ramDiskClearAfterRestart)
	: 
	m_rom(), m_ram(),
	m_pathRamDiskData(_pathRamDiskData),
	m_ramDiskClearAfterRestart(_ramDiskClearAfterRestart)
{
	auto res = dev::LoadFile(dev::GetExecutableDir() + _pathBootData);
	if (res) m_rom = *res;

	res = dev::LoadFile(dev::GetExecutableDir() + _pathRamDiskData);
	if (res) {
		RamDiskData ramDiskData = *res;
		ramDiskData.resize(MEMORY_RAMDISK_LEN * RAM_DISK_MAX);
		std::copy(ramDiskData.begin(), ramDiskData.end(), m_ram.data() + MEMORY_MAIN_LEN);
	}
}

dev::Memory::~Memory()
{
	// store RamDisk
	RamDiskData ramDiskData(m_ram.begin() + MEMORY_MAIN_LEN, m_ram.end());
	dev::SaveFile(dev::GetExecutableDir() + m_pathRamDiskData, ramDiskData, true);
}
void dev::Memory::Init()
{
	if (m_ramDiskClearAfterRestart){
		m_ram.fill(0);
	}
	else {
		std::fill(m_ram.data(), m_ram.data() + MEMORY_MAIN_LEN, 0);
	}


	*((uint64_t*) &m_mappings[0].data) = 0;
	m_state.update.mapping.data = m_state.update.ramdiskIdx = m_mappingsEnabled = 0;
	m_state.update.memType = MemType::ROM;
	m_state.ramP = &m_ram;
}

void dev::Memory::InitRamDiskMapping()
{
	*((uint64_t*) &m_mappings[0].data) = 0;
	m_state.update.mapping.data = 0;
	m_state.update.ramdiskIdx = 0;
	m_mappingsEnabled = 0;
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

void dev::Memory::SetByteGlobal(const GlobalAddr _globalAddr, const uint8_t _data)
{
	m_ram[_globalAddr] = _data;
}

auto dev::Memory::GetByteGlobal(const GlobalAddr _globalAddr) const 
-> uint8_t
{
	return m_ram[_globalAddr];
}

auto dev::Memory::GetByte(const Addr _addr, const AddrSpace _addrSpace) const
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

void dev::Memory::CpuInvokesRst7()
{
	m_state.debug.instr[0] = 0xFF; // OPCODE_RST7
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
	m_state.debug.beforeWrite[_byteNum] = m_ram[globalAddr];
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
	if (!(m_state.update.mapping.data & MAPPING_MODE_MASK)) return _addr;

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

	for (int ramdiskIdx = 0; ramdiskIdx < RAM_DISK_MAX; ramdiskIdx++)
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