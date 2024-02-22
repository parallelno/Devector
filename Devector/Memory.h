#pragma once
#ifndef DEV_MEMORY_H
#define DEV_MEMORY_H

#include <cstdint>
#include <vector>

namespace dev
{
	class Memory
	{
	public: 
		static constexpr uint32_t ROM_LOAD_ADDR = 0x100;

		static constexpr uint32_t MEMORY_MAIN_LEN = 64 * 1024;
		static constexpr uint32_t MEMORY_RAMDISK_LEN = 256 * 1024;
		static constexpr uint32_t RAM_DISK_PAGE_LEN = 64 * 1024;
		static constexpr uint32_t RAMDISK_MAX = 1;

		static constexpr uint32_t GLOBAL_MEMORY_LEN = MEMORY_MAIN_LEN + MEMORY_RAMDISK_LEN * RAMDISK_MAX;
		int8_t m_data[GLOBAL_MEMORY_LEN];

		enum AddrSpace
		{
			RAM, STACK, GLOBAL
		};

		bool m_mappingModeStack;
		uint32_t m_mappingPageStack;
		uint8_t m_mappingModeRam;
		uint32_t m_mappingPageRam;

		void Init();
		void Load(const std::vector<uint8_t>& _data);

		auto GetByte(uint32_t _addr, Memory::AddrSpace _addr_space = Memory::AddrSpace::RAM) const -> uint8_t;
		void SetByte(uint32_t _addr, uint8_t _value, Memory::AddrSpace _addr_space = Memory::AddrSpace::RAM);

		int GetWord(uint32_t _addr, Memory::AddrSpace _addr_space = Memory::AddrSpace::RAM) const;
		int Length();
		uint32_t GetGlobalAddr(uint32_t _addr, Memory::AddrSpace _addr_space) const;

	};
}
#endif // !DEV_MEMORY_H