#pragma once
#ifndef DEV_MEMORY_H
#define DEV_MEMORY_H

#include <cstdint>
#include <vector>
#include <functional>

#include "Types.h"

namespace dev
{
	class Memory
	{
	public: 
		static constexpr uint8_t MAPPING_MODE_RAM_OFF	= 0;
		static constexpr uint8_t MAPPING_MODE_RAM_A000	= 1 << 0;
		static constexpr uint8_t MAPPING_MODE_RAM_8000	= 1 << 1;
		static constexpr uint8_t MAPPING_MODE_RAM_E000	= 1 << 2;

		static constexpr GlobalAddr ROM_LOAD_ADDR = 0x100;

		static constexpr size_t MEMORY_MAIN_LEN = 64 * 1024;
		static constexpr size_t MEMORY_RAMDISK_LEN = 256 * 1024;
		static constexpr size_t RAM_DISK_PAGE_LEN = 64 * 1024;
		static constexpr size_t RAMDISK_MAX = 1;

		static constexpr size_t GLOBAL_MEMORY_LEN = MEMORY_MAIN_LEN + MEMORY_RAMDISK_LEN * RAMDISK_MAX;
		int8_t m_data[GLOBAL_MEMORY_LEN];

		enum AddrSpace
		{
			RAM, STACK, GLOBAL
		};

		bool m_mappingModeStack;
		size_t m_mappingPageStack;
		uint8_t m_mappingModeRam;	// 0 - no mapping, 
									// MAPPING_MODE_RAM_A000 - addr range [0xA000-0xDFFF] is mapped to the ram-disk 
									// MAPPING_MODE_RAM_8000 - addr range [0x8000-0x9FFF] is mapped to the ram-disk 
									// MAPPING_MODE_RAM_E000 - addr range [0xE000-0xFFFF] is mapped to the ram-disk
		uint8_t m_mappingPageRam; // 0 - mapping to the ram-disk page0, etc

		void Init();
		void Load(const std::vector<uint8_t>& _data);

		auto GetByte(GlobalAddr _globalAddr, Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM) const -> uint8_t;
		void SetByte(GlobalAddr _globalAddr, uint8_t _value, Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);

		auto GetWord(GlobalAddr _globalAddr, Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM) const -> uint16_t;
		int Length();
		auto GetGlobalAddr(const GlobalAddr _globalAddr, AddrSpace _addrSpace) const -> GlobalAddr;
		bool IsRamMapped(const Addr _addr) const;
	};
}
#endif // !DEV_MEMORY_H