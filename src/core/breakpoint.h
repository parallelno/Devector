#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <format>
#include <bit>

#include "utils/types.h"
#include "core/cpu_i8080.h"
#include "core/memory.h"

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
		enum class Status : uint8_t {
			DISABLED = 0,
			ACTIVE,
			DELETED,
			COUNT,
		};
		static constexpr int STATUS_BIT_WIDTH = std::bit_width<uint8_t>(static_cast<uint8_t>(Status::COUNT) - 1);

		enum class Operand : uint8_t { A = 0, F, B, C, D, E, H, L, PSW, BC, DE, HL, CC, SP, COUNT };
		static constexpr int OPERAND_BIT_WIDTH = std::bit_width<uint8_t>(static_cast<uint8_t>(Operand::COUNT) -1);

#pragma pack(push, 1)
		struct DataStruct {
			MemPages memPages;
			uint64_t value;
			Addr addr : 16;
			Operand operand : OPERAND_BIT_WIDTH;
			Condition cond : CONDITION_BIT_WIDTH + 1; // +1 to avoid warnings about bits not enough (it's enough, COUNT used only for counting elements)
			Status status : STATUS_BIT_WIDTH;
			bool autoDel : 1;

			DataStruct(
				const Addr _addr,
				const MemPages _memPages = MAPPING_PAGES_ALL,
				const Status _status = Status::ACTIVE,
				const bool _autoDel = false,
				const Operand _operand = Operand::A,
				const Condition _cond = Condition::ANY,
				const size_t _value = 0
			) :
				memPages(_memPages), value(_value), addr(_addr), operand(_operand), cond(_cond), status(_status), autoDel(_autoDel)
			{};
		};

		union Data {
			DataStruct structured;
			struct {
				uint64_t data0;
				uint64_t data1;
				uint32_t data2;
			};

			Data(
				const Addr _addr,
				const MemPages _memPages = MAPPING_PAGES_ALL,
				const Status _status = Status::ACTIVE,
				const bool _autoDel = false,
				const Operand _operand = Operand::A,
				const Condition _cond = Condition::ANY,
				const size_t _value = 0
			) :
				structured(_addr, _memPages, _status, _autoDel, _operand, _cond, _value)
			{};
			Data(const uint64_t _data0, const uint64_t _data1, const uint32_t _data2)
				: 
				data0(_data0), data1(_data1), data2(_data2)
			{}
		};
#pragma pack(pop)

		Breakpoint(Data&& _data, const std::string& _comment = "");

		void Update(Breakpoint&& _bp);

		auto GetAddrMappingS() const -> const char*;
		bool IsActive() const { return data.structured.status == Status::ACTIVE; };
		bool CheckStatus(const CpuI8080::State& _cpuState, const Memory::State& _memState) const;
		auto GetOperandS() const -> const char*;
		auto GetConditionS() const -> const std::string;
		void Print() const;
		auto IsActiveS() const -> const char*;
		void UpdateAddrMappingS();

		Data data;
		std::string comment;

		std::string addrMappingS;
	};
}