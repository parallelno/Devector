#pragma once

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <format>

#include "Utils/Types.h"
#include "Core/CpuI8080.h"
#include "Core/Memory.h"

namespace dev
{
	static const char* bpOperandsS[] = { 
		"A", "Flags", "B", "C", "D", "E", 
		"H", "L", "PSW", "BC", "DE", "HL", "CC", "SP" };

	struct Breakpoint
	{
#pragma pack(push, 1)
		union MemPages {
			struct {
				uint8_t ram  : 1; // main ram
				uint8_t rd00 : 1; // Ram-disk0 page 0
				uint8_t rd01 : 1; // Ram-disk0 page 1
				uint8_t rd02 : 1; // Ram-disk0 page 2
				uint8_t rd03 : 1; // Ram-disk0 page 3
				uint8_t rd10 : 1; // Ram-disk1 page 0
				uint8_t rd11 : 1; // Ram-disk1 page 1
				uint8_t rd12 : 1; // Ram-disk1 page 2
				uint8_t rd13 : 1; // Ram-disk1 page 3
				uint8_t rd20 : 1; // Ram-disk2 page 0
				uint8_t rd21 : 1; // Ram-disk2 page 1
				uint8_t rd22 : 1; // Ram-disk2 page 2
				uint8_t rd23 : 1; // Ram-disk2 page 3
				uint8_t rd30 : 1; // Ram-disk3 page 0
				uint8_t rd31 : 1; // Ram-disk3 page 1
				uint8_t rd32 : 1; // Ram-disk3 page 2
				uint8_t rd33 : 1; // Ram-disk3 page 3
			};
			uint32_t data;  // ...876543210R, R - ram, 0 - Ram-disk0 page0, 1 - Ram-disk0 page1,... 4 - Ram-disk1 page0, 5 - Ram-disk0 page1 etc
			MemPages(uint32_t _data) : data(_data) {};
		};
#pragma pack(pop)

		static constexpr uint32_t MAPPING_PAGES_ALL = -1;
		enum class Status : int {
			DISABLED = 0,
			ACTIVE,
			DELETED,
		};

		enum class Operand : uint8_t { A = 0, F, B, C, D, E, H, L, PSW, BC, DE, HL, CC, SP };
		
		auto GetAddr() const -> Addr { return addr; };
		auto GetAddrMappingS() const -> const char*;
		inline bool IsActive() const { return status == Status::ACTIVE; };

		Breakpoint(const Addr _addr,
			const MemPages _memPages = MAPPING_PAGES_ALL,
			const Status _status = Status::ACTIVE, 
			const bool _autoDel = false, 
			const Operand _op = Operand::A,
			const Condition _cond = Condition::ANY,
			const size_t _val = 0,
			const std::string& _comment = "");
		void Update(const Addr _addr,
			const MemPages _memPages = MAPPING_PAGES_ALL,
			const Status _status = Status::ACTIVE,
			const bool _autoDel = false, 
			const Operand _op = Operand::A,
			const Condition _cond = Condition::ANY,
			const size_t _val = 0, const std::string& _comment = "");

		bool CheckStatus(const CpuI8080::State& _cpuState, const Memory::State& _memState) const;
		auto GetOperandS() const -> const char*;
		auto GetConditionS() const -> const std::string;
		void Print() const;
		auto IsActiveS() const -> const char*;

		Addr addr;
		MemPages memPages;
		Status status;
		bool autoDel;
		Operand operand;
		Condition cond;
		uint64_t value;
		std::string comment;
	};
}