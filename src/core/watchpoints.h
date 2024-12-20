#pragma once

#include <string>

#include "utils/types.h"
#include "core/watchpoint.h"

namespace dev
{
	struct Watchpoints
	{
public:
		using WpMap = std::unordered_map<dev::Id, Watchpoint>;

		void Add(Watchpoint&& _bp);
		void Del(const dev::Id _id);
		void Check(const Watchpoint::Access _access, const GlobalAddr _globalAddr, const uint8_t _value);
		auto GetAll() -> const WpMap&;
		auto GetUpdates() -> const uint32_t;
		void Clear();
		void Reset();
		bool CheckBreak();

private:

		WpMap m_wps;
		uint32_t m_updates = 0; // counts number of updates
		bool m_wpBreak = false;
		std::string addrMappingS;
	};
}