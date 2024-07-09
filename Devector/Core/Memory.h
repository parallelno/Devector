#pragma once

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
		enum AddrSpace : uint8_t { RAM = 0, STACK = 1 };
		enum class MemType : uint8_t { ROM = 0, RAM };

		static constexpr GlobalAddr ROM_LOAD_ADDR = 0x100;

		static constexpr size_t MEM_64K = 64 * 1024;
		static constexpr size_t RAM_DISK_PAGE_LEN = MEM_64K;
		static constexpr size_t MEMORY_RAMDISK_LEN = 4 * MEM_64K;
		static constexpr size_t RAMDISK_MAX = 2;

		static constexpr size_t MEMORY_MAIN_LEN = MEM_64K;
		static constexpr size_t GLOBAL_MEMORY_LEN = MEMORY_MAIN_LEN + MEMORY_RAMDISK_LEN * RAMDISK_MAX;

		using Rom = std::vector<uint8_t>;
		using Ram = std::array<uint8_t, GLOBAL_MEMORY_LEN>;

		static constexpr uint8_t MAPPING_RAM_MODE_MASK = 0b11100000;
		static constexpr uint8_t MAPPING_MODE_MASK = 0b11110000;

#pragma pack(push, 1)
		union Mapping {
			struct {
				uint8_t pageRam : 2;	// Ram-Disk 64k page idx accesssed via non-stack instructions (all instructions except mentioned below)
				uint8_t pageStack : 2;	// Ram-Disk 64k page idx accesssed via the stack instructions (Push, Pop, XTHL, Call, Ret, C*, R*, RST)
				uint8_t modeStack : 1;		// enabling stack mapping
				bool modeRamA : 1; // enabling ram [0xA000-0xDFFF] mapped into the the Ram-Disk
				bool modeRam8 : 1; // enabling ram [0x8000-0x9FFF] mapped into the the Ram-Disk
				bool modeRamE : 1; // enabling ram [0xE000-0xFFFF] mapped into the the Ram-Disk
			};
			uint8_t data = 0;
			Mapping(const uint8_t _data = 0) : data(_data) {}
		};
#pragma pack(pop)
		// contains the data stored in memory by the last executed instruction
#pragma pack(push, 1)
		struct Update
		{
			Addr addr : 16 = 0;
			uint8_t b1 : 8 = 0;
			uint8_t b2 : 8 = 0;
			uint8_t len : 2 = 0;
			uint8_t stack : 1 = 0; // 0: b2 addr = addr+1, 1: b2 addr = addr-1
			MemType memType : 1 = MemType::RAM;
		};
#pragma pack(pop)

#pragma pack(push, 1)
		struct State
		{
			Mapping mapping1; // mapping to the Ram-disk 1
			Mapping mapping2; // mapping to the Ram-disk 2
			Update update;
		};
#pragma pack(pop)

		Memory(const std::wstring& _pathBootData);
		void Init();
		// internal thread access
		void SetMemType(const MemType _memType);
		void SetRam(const Addr _addr, const std::vector<uint8_t>& _data);
		auto GetByte(const Addr _addr,
			const Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM) -> uint8_t;
		void SetByte(const Addr _addr, uint8_t _value,
			const Memory::AddrSpace _addrSpace, const uint8_t _byteNum);
		auto GetScreenBytes(Addr _screenAddrOffset) const -> uint32_t;
		auto GetRam() const -> const Ram*;
		auto GetGlobalAddr(Addr _addr, 
			const AddrSpace _addrSpace) const -> GlobalAddr;
		auto GetState() const -> const State&;
		auto IsRamMapped(const Mapping _mapping, const Addr _addr) const -> GlobalAddr;
		bool IsRomEnabled() const;
		void SetRamDiskMode(uint8_t _diskIdx, uint8_t _data);
		void Restart();

	private:
		Ram m_ram;
		Rom m_rom;
		State m_state;
	};
}