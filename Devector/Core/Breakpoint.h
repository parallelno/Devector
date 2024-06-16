#pragma once

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <format>

#include "Utils/Types.h"
#include "Core/CpuI8080.h"

namespace dev
{
	static const char* bpOperandsS[] = { 
		"A", "Flags", "B", "C", "D", "E", 
		"H", "L", "PSW", "BC", "DE", "HL", "CC", "SP" };

	static const char* bpCondsS[] = {
	"ANY", "EQU", "LESS", "EQU", "LESS",
	"GREATER", "LESS_EQU", "GREATER_EQU",
	"NOT_EQU" };

	struct Breakpoint
	{
		static constexpr uint8_t MAPPING_RAM		   = 1 << 0;
		static constexpr uint8_t MAPPING_RAMDISK_PAGE0 = 1 << 1;
		static constexpr uint8_t MAPPING_RAMDISK_PAGE1 = 1 << 2;
		static constexpr uint8_t MAPPING_RAMDISK_PAGE2 = 1 << 3;
		static constexpr uint8_t MAPPING_RAMDISK_PAGE3 = 1 << 4;
		static constexpr uint8_t MAPPING_PAGES_ALL =  // 43210R, R - ram, 0 - ram disk 0 page, 1 - ram disk 1 page, etc
											MAPPING_RAM | 
											MAPPING_RAMDISK_PAGE0 | 
											MAPPING_RAMDISK_PAGE1 |
											MAPPING_RAMDISK_PAGE2 |
											MAPPING_RAMDISK_PAGE3;
		enum class Status : int {
			DISABLED = 0,
			ACTIVE,
			DELETED,
		};

		enum class Operand : uint8_t { A = 0, F, B, C, D, E, H, L, PSW, BC, DE, HL, CC, SP };
		enum class Condition : uint8_t { ANY = 0, EQU, LESS, GREATER, LESS_EQU, GREATER_EQU, NOT_EQU };
		
		auto GetAddr() const -> Addr { return addr; };
		auto GetAddrMappingS() const -> const char*;
		inline bool IsActive() const { return status == Status::ACTIVE; };

		Breakpoint(const Addr _addr,
			const uint8_t _mappingPages = MAPPING_PAGES_ALL,
			const Status _status = Status::ACTIVE, 
			const bool _autoDel = false, 
			const Operand _op = Operand::A,
			const Condition _cond = Condition::ANY,
			const size_t _val = 0,
			const std::string& _comment = "");
		void Update(const Addr _addr,
			const uint8_t _mappingPages = MAPPING_PAGES_ALL,
			const Status _status = Status::ACTIVE,
			const bool _autoDel = false, 
			const Operand _op = Operand::A,
			const Condition _cond = Condition::ANY,
			const size_t _val = 0, const std::string& _comment = "");

		bool CheckStatus(const CpuI8080::State& _state, 
			const uint8_t _mappingModeRam,
			const uint8_t _mappingPageRam) const;
		auto GetOperandS() const -> const char*;
		auto GetConditionS() const -> const char*;
		void Print() const;
		auto IsActiveS() const -> const char*;

		Addr addr;
		uint8_t mappingPages; // 3210R, R - ram, 0 - ram disk 0 page, 1 - ram disk 1 page, etc
		Status status;
		bool autoDel;
		Operand operand;
		Condition cond;
		uint64_t value;
		std::string comment;
	};
}