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
				uint8_t ram  : 1;
				uint8_t rdisk0page0 : 1;
				uint8_t rdisk0page1 : 1;
				uint8_t rdisk0page2 : 1;
				uint8_t rdisk0page3 : 1;
				uint8_t rdisk1page0 : 1;
				uint8_t rdisk1page1 : 1;
				uint8_t rdisk1page2 : 1;
				uint8_t rdisk1page3 : 1;
				uint8_t rdisk2page0 : 1;
				uint8_t rdisk2page1 : 1;
				uint8_t rdisk2page2 : 1;
				uint8_t rdisk2page3 : 1;
				uint8_t rdisk3page0 : 1;
				uint8_t rdisk3page1 : 1;
				uint8_t rdisk3page2 : 1;
				uint8_t rdisk3page3 : 1;
				uint8_t rdisk4page0 : 1;
				uint8_t rdisk4page1 : 1;
				uint8_t rdisk4page2 : 1;
				uint8_t rdisk4page3 : 1;
				uint8_t rdisk5page0 : 1;
				uint8_t rdisk5page1 : 1;
				uint8_t rdisk5page2 : 1;
				uint8_t rdisk5page3 : 1;
				uint8_t rdisk6page0 : 1;
				uint8_t rdisk6page1 : 1;
				uint8_t rdisk6page2 : 1;
				uint8_t rdisk6page3 : 1;
				uint8_t rdisk7page0 : 1;
				uint8_t rdisk7page1 : 1;
				uint8_t rdisk7page2 : 1;
				uint8_t rdisk7page3 : 1;

			};
			uint64_t data;  // ...876543210R, R - ram, 0 - Ram-disk0 page0, 1 - Ram-disk0 page1,... 4 - Ram-disk1 page0, 5 - Ram-disk0 page1 etc
			MemPages(uint64_t _data) : data(_data) {};
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
		void UpdateAddrMappingS();

		Addr addr;
		MemPages memPages;
		Status status;
		bool autoDel;
		Operand operand;
		Condition cond;
		uint64_t value;
		std::string comment;
		std::string addrMappingS;
	};
}