#pragma once
#ifndef DEV_MEMORY_H
#define DEV_MEMORY_H

#include <cstdint>
#include <vector>
#include <array>
#include <functional>
#include <mutex>

#include "Utils/Types.h"

namespace dev
{
	class Memory
	{
	public:
		enum AddrSpace
		{
			RAM, STACK, GLOBAL
		};
		enum class MemType {
			ROM,
			RAM
		};

		static constexpr uint8_t MAPPING_RAM_MODE_A000 = 1 << 5;
		static constexpr uint8_t MAPPING_RAM_MODE_8000 = 1 << 6;
		static constexpr uint8_t MAPPING_RAM_MODE_E000 = 1 << 7;
		static constexpr uint8_t MAPPING_RAM_MODE_MASK = MAPPING_RAM_MODE_A000 | MAPPING_RAM_MODE_8000 | MAPPING_RAM_MODE_E000;
		static constexpr uint8_t MAPPING_RAM_PAGE_MASK = 0b11;

		static constexpr uint8_t MAPPING_STACK_MODE_MASK = 0b10000;
		static constexpr uint8_t MAPPING_STACK_PAGE_MASK = 0b1100;

		static constexpr GlobalAddr ROM_LOAD_ADDR = 0x100;

		static constexpr size_t MEM_64K = 64 * 1024;
		static constexpr size_t RAM_DISK_PAGE_LEN = MEM_64K;
		static constexpr size_t MEMORY_RAMDISK_LEN = 4 * MEM_64K;
		static constexpr size_t RAMDISK_MAX = 1;

		static constexpr size_t MEMORY_MAIN_LEN = MEM_64K;
		static constexpr size_t GLOBAL_MEMORY_LEN = MEMORY_MAIN_LEN + MEMORY_RAMDISK_LEN * RAMDISK_MAX;

		using Rom = std::vector<uint8_t>;
		using Ram = std::array<uint8_t, GLOBAL_MEMORY_LEN>;

		bool m_mappingModeStack = false;
		size_t m_mappingPageStack = 0;
		uint8_t m_mappingModeRam = 0;	// 0 - no mapping, 
									// MAPPING_RAM_MODE_A000 - addr range [0xA000-0xDFFF] is mapped to the ram-disk 
									// MAPPING_RAM_MODE_8000 - addr range [0x8000-0x9FFF] is mapped to the ram-disk 
									// MAPPING_RAM_MODE_E000 - addr range [0xE000-0xFFFF] is mapped to the ram-disk
		uint8_t m_mappingPageRam = 0; // 0 - mapping to the ram-disk page0, etc

		Memory(const std::wstring& _pathBootData);
		void Init();
		// internal thread access
		void SetMemType(const MemType _memType);
		void SetRam(const std::vector<uint8_t>& _data, const Addr _addr = ROM_LOAD_ADDR);
		auto GetByte(GlobalAddr _globalAddr, const Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM) -> uint8_t;
		void SetByte(GlobalAddr _globalAddr, uint8_t _value, const Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);
		auto GetWord(GlobalAddr _globalAddr, const Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM) -> uint16_t;
		auto GetScreenBytes(Addr _screenAddrOffset) const -> uint32_t;
		auto GetRam() const -> const Ram*;
		auto GetGlobalAddr(GlobalAddr _globalAddr, const AddrSpace _addrSpace) const -> GlobalAddr;
		bool IsRamMapped(const Addr _addr) const;
		bool IsRomEnabled() const;
		void SetRamDiskMode(uint8_t _data);
		void Restart();

	private:
		Ram m_ram;
		Rom m_rom;

		MemType m_memType = MemType::ROM;
	};
}
#endif // !DEV_MEMORY_H