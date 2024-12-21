#pragma once

#include <string>

#include "utils/types.h"
#include "utils/json_utils.h"
#include "core/breakpoint.h"

namespace dev
{
	struct Breakpoints
	{
public:
		using BpMap = std::unordered_map<GlobalAddr, Breakpoint>;

		void SetStatus(const Addr _addr, const Breakpoint::Status _status);
		void Add(Breakpoint&& _bp);
		void Add(const nlohmann::json& _bpJ);
		void Del(const Addr _addr);
		bool Check(const CpuI8080::State& _cpuState, const Memory::State& _memState);
		auto GetAll() -> const BpMap&;
		auto GetUpdates() -> const uint32_t;
		auto GetStatus(const Addr _addr) -> const Breakpoint::Status;
		void Clear();
		void Reset();

private:

		BpMap m_bps;
		uint32_t m_updates; // counts number of updates

		std::string addrMappingS;
	};
}