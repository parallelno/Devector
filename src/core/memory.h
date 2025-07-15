#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <functional>
#include <mutex>
#include <string>
#include <format>

#include "utils/types.h"

namespace dev
{
	class Memory
	{
	public:
		#include "core/memory_consts.h"

		using Rom = std::vector<uint8_t>;
		using Ram = std::array<uint8_t, MEMORY_GLOBAL_LEN>;
		using RamDiskData = std::vector<uint8_t>;

#pragma pack(push, 1)

		//=============================================================================
		// The RAM Disk
		//=============================================================================
		// The RAM Disk is an external 256-byte RAM space. It is split into four 64k 
		// memory pages. It operates in two modes:
		// 
		//   • Stack Mode:
		//       - Accessed using stack-related instructions
		// 
		//   • Memory-Mapped Mode:
		//       - The RAM Disk is mapped into the 0xA000–0xFFFF address space
		//       - The RAM Disk memory space operates as a regular RAM
		//
		// Stack Mode is applied if:
		//   - Stack mapping is enabled
		//   - The RAM is accessed via stack-related instructions:
		//       Push, Pop, XTHL, Call, Ret, conditional calls/returns, or RST

		// Memory-Mapped Mode is applied if:
		//	- RAM mapping is enabled
		//	- The address falls within the RAM mapping range associated with the current
		//       RAM mapping
		//	- The RAM is accessed via non-stack instructions when Stack Mode is enabled
		//   - The RAM is accessed via stack instructions when Stack Mode is disabled
		//		Example:
		//		Memory-Mapped Mode of the first RAM Disk is enabled.
		//		Memory Mapping is enabled for 0xA000-0xDFFF.
		//		Stack Mode is disabled.
		//		SP = 0xA000
		//		POP H
		//		In that case, the Memory-Mapped Mode is used to access the RAM Disk.

		// The RAM Disk activation command: %E8ASssMM:
		//	MM: The index of the RAM Disk 64k page accessed in the Memory-Mapped Mode
		//	ss: The index of the RAM Disk 64k page accessed in the Stack Mode
		//	S : Enables the Stack Mode
		//	A : Enables the Memory-Mapped Mode with mapping for range [0xA000-0xDFFF]
		//	8 : Enables the Memory-Mapped Mode with mapping for range [0x8000-0x9FFF]
		//	E : Enables the Memory-Mapped Mode with mapping for range [0xE000-0xFFFF]

		union Mapping {
			struct {
				uint8_t pageRam : 2;	// The index of the RAM Disk 64k page accessed in the Memory-Mapped Mode
				uint8_t pageStack : 2;	// The index of the RAM Disk 64k page accessed in the Stack Mode
				bool modeStack : 1;		// Enables the Stack Mode
				bool modeRamA : 1; // Enables the Memory-Mapped Mode with mapping for range [0xA000-0xDFFF]
				bool modeRam8 : 1; // Enables the Memory-Mapped Mode with mapping for range [0x8000-0x9FFF]
				bool modeRamE : 1; // Enables the Memory-Mapped Mode with mapping for range [0xE000-0xFFFF]
			};
			uint8_t data = 0;
			Mapping(const uint8_t _data = 0) : data(_data) {}

			auto ToStr() const -> std::string
			{
				return std::format("mapping: ram mode:{}{}{}, stack mode:{}, ram page:{}, stack page:{}",
					modeRam8 ? "8" : "-",
					modeRamA ? "AC" : "--",
					modeRamE ? "E" : "-",
					modeStack ? "S" : "-",
					(int)pageRam, (int)pageStack);
			}
			auto RamModeToStr() const -> std::string
			{
				auto modeA = modeRamA ? "AC" : "--";
				auto mode8 = modeRam8 ? "8" : "-";
				auto modeE = modeRamE ? "E" : "-";
				return std::format("{}{}{}", mode8, modeA, modeE);
			}

		};
#pragma pack(pop)

#pragma pack(push, 1)
		Mapping m_mappings[RAM_DISK_MAX];
#pragma pack(pop)

		// contains the data stored in memory by the last executed instruction
#pragma pack(push, 1)
		struct Update
		{
			Mapping mapping;
			uint8_t ramdiskIdx : 3 = 0; // RAM_DISK_MAX = 8
			MemType memType : 1 = MemType::RAM;
		};
#pragma pack(pop)

#pragma pack(push, 1)
		struct Debug
		{
			GlobalAddr instrGlobalAddr;
			uint8_t instr[3];
			uint8_t instrLen = 0;
			GlobalAddr readGlobalAddr[2];
			uint8_t read[2];
			uint8_t readLen = 0;
			GlobalAddr writeGlobalAddr[2];
			uint8_t write[2];
			uint8_t writeLen = 0;
			uint8_t beforeWrite[2];
			inline void Init() { instrLen = readLen = writeLen= 0; }
		};
#pragma pack(pop)

#pragma pack(push, 1)
		struct State
		{
			Debug debug;
			Update update;
			Ram* ramP = nullptr;
		};
#pragma pack(pop)

		Memory(const std::string& _pathBootData, const std::string& _pathRamDiskData, const bool _ramDiskClearAfterRestart);
		~Memory();
		void Init();
		void Restart();
		auto GetByte(const Addr _addr,
			const Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM) const -> uint8_t;
		auto GetByteGlobal(const GlobalAddr _globalAddr) const -> uint8_t;			
		auto CpuReadInstr(const Addr _addr,
			const Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM,
			const uint8_t _byteNum = 0) -> uint8_t;
		void CpuInvokesRst7();
		auto CpuRead(const Addr _addr,
			const Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM,
			const uint8_t _byteNum = 0) -> uint8_t;
		void CpuWrite(const Addr _addr, uint8_t _value,
			const Memory::AddrSpace _addrSpace, const uint8_t _byteNum);
		auto GetScreenBytes(Addr _screenAddrOffset) const -> uint32_t;
		auto GetRam() const -> const Ram*;
		auto GetGlobalAddr(const Addr _addr, const AddrSpace _addrSpace) const -> GlobalAddr;
		auto GetState() const -> const State& { return m_state; };
		auto GetStateP() -> State* { return &m_state; };
		auto GetMappingsP() const -> const Mapping* { return m_mappings; };
		void SetRamDiskMode(uint8_t _diskIdx, uint8_t _data);
		void InitRamDiskMapping();
		void SetMemType(const MemType _memType);
		void SetRam(const Addr _addr, const std::vector<uint8_t>& _data);
		void SetByteGlobal(const GlobalAddr _addr, const uint8_t _data);
		bool IsException();
		bool IsRomEnabled() const;
		inline void DebugInit() { m_state.debug.Init(); };

	private:

		Ram m_ram;
		Rom m_rom;
		State m_state;
		int m_mappingsEnabled = 0;
		std::string m_pathRamDiskData;
		bool m_ramDiskClearAfterRestart = true;
	};
}