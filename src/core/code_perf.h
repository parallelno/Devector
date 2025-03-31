#pragma once

#include <string>
#include <format>

#include "utils/types.h"
#include "utils/str_utils.h"
#include "utils/json_utils.h"

namespace dev
{
	struct CodePerf
	{
		static constexpr int TESTS_MAX = 20000;
		std::string label;
		Addr addrStart = 0;
		Addr addrEnd = 0x100;
		double averageCcDiff = 0.0f; // average cc (Cpu CLock) difference between it entered the addrStart and exited addrEnd
		int64_t tests = 0; // how many testes executed
		int64_t cc = 0; // the current perf test cc
		bool active = true;

		auto AddrToStr() const -> std::string {
			return std::format("0x{:06x}-0x{:06x}: {}, average cc: {}, tests: {}",
				addrStart, addrEnd,
				active ? "active" : "not active",
				static_cast<int>(std::round(averageCcDiff)),
				tests);
		}

		void Erase()
		{
			label.clear();
			addrStart = 0;
			addrEnd = 0x100;
			averageCcDiff = 0;
			tests = 0;
			cc = 0;
			active = true;
		}

		void CheckPerf(const Addr _addr, const uint64_t _cc)
		{
			if (!active || (addrStart != _addr && cc == 0)) return;

			if (addrStart == _addr){
				cc = _cc;
			}
			else
			if (addrEnd == _addr)
			{
					tests += tests >= TESTS_MAX ? 0 : 1;
					auto weight = 1.0 / tests;
					int64_t ccDiff = _cc - cc;
					averageCcDiff += (ccDiff - averageCcDiff) * weight;
					cc = 0;
			}
		}

		CodePerf() = default;

		CodePerf(const nlohmann::json& _json)
			:
			label(_json["label"].get<std::string>()),
			addrStart(dev::StrHexToInt(_json["addrStart"].get<std::string>().c_str())),
			addrEnd(dev::StrHexToInt(_json["addrEnd"].get<std::string>().c_str())),
			active(_json["active"].get<bool>())
		{}

		auto ToJson() const -> nlohmann::json
		{
			return {
				{"label", label},
				{"addrStart", std::format("0x{:04X}", addrStart)},
				{"addrEnd", std::format("0x{:04X}", addrEnd)},
				{"active", active}
			};
		}
	};
}