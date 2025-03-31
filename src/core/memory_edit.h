#pragma once

#include <string>
#include <format>

#include "utils/types.h"
#include "utils/str_utils.h"
#include "utils/json_utils.h"

namespace dev
{
	struct MemoryEdit
	{
		std::string comment;
		GlobalAddr globalAddr = 0;
		uint8_t value = 0;
		bool readonly = true; // if true, memory is not modified
		bool active = true;

		auto AddrToStr() const -> std::string { return std::format("0x{:06x}: 0x{:02x} {}, {}", globalAddr, value, readonly ? "read-only" : "", active ? "active" : "not active"); }
		void Erase()
		{
			comment.clear();
			globalAddr = 0;
			value = 0;
			readonly = true;
			active = true;
		}

		auto ToJson() const -> nlohmann::json
		{
			return {
				{"comment", comment},
				{"globalAddr", std::format("0x{:06X}", globalAddr)},
				{"value", std::format("0x{:02X}", value)},
				{"readonly", readonly},
				{"active", active}
			};
		}

		MemoryEdit() = default;

		MemoryEdit(const nlohmann::json& _json)
			:
			comment(_json["comment"].get<std::string>()),
			globalAddr(dev::StrHexToInt(_json["globalAddr"].get<std::string>().c_str())),
			value(dev::StrHexToInt(_json["value"].get<std::string>().c_str())),
			readonly(_json["readonly"].get<bool>()),
			active(_json["active"].get<bool>())
		{}

		MemoryEdit(GlobalAddr _globalAddr, uint8_t _value, const std::string& _comment = "", bool _readonly = true, bool _active = true)
			:
			comment(_comment),
			globalAddr(_globalAddr),
			value(_value),
			readonly(_readonly),
			active(_active) {}
	};
}