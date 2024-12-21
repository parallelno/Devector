#pragma once

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <format>

#include "utils/types.h"
#include "core/cpu_i8080.h"
#include "core/memory.h"
#include "utils/json_utils.h"

namespace dev
{
	static Id watchpointId = 0;
	static const char* wpAccessS[] = { "R", "W", "RW" };
	static const char* wpTypesS[] = { "LEN", "WORD" };

	struct Watchpoint
	{
		// LEN - breaks if the condition succeds for any bytes in m_len range
		// WORD - breaks if the condition succeds for a word
		enum class Type : uint8_t { LEN = 0, WORD, COUNT };
		static constexpr int TYPE_BIT_WIDTH = std::bit_width<uint8_t>(static_cast<uint8_t>(Type::COUNT) - 1);
		enum class Access : uint8_t { R = 0, W, RW, COUNT };
		static constexpr int ACCESS_BIT_WIDTH = std::bit_width<uint8_t>(static_cast<uint8_t>(Access::COUNT) - 1);

#pragma pack(push, 1)
		union Data {
			struct {
				GlobalAddr globalAddr;
				Id id;
				GlobalAddr len;
				uint16_t value;

				Access access	: ACCESS_BIT_WIDTH;
				Condition cond	: CONDITION_BIT_WIDTH + 1;
				Type type		: TYPE_BIT_WIDTH + 1;

				bool active		: 1;
				bool breakL		: 1;
				bool breakH		: 1;
			};
			struct {
				uint64_t data0;
				uint64_t data1;
			};

			Data(
				const Id _id, const Access _access, const GlobalAddr _globalAddr, const Condition _cond,
				const uint16_t _value, const Type _type = Type::LEN, const GlobalAddr _len = 1,
				const bool _active = true,
				const bool _breakH = false, const bool _breakL = false
			) :
				id(_id == -1 ? watchpointId++ : _id), access(_access), globalAddr(_globalAddr), 
				cond(_cond), value(_value), type(_type), len(_len), active(_active), breakH(_breakH), breakL(_breakL)
			{};
			Data(const uint64_t _data0, const uint64_t _data1)
				:
				data0(_data0), data1(_data1)
			{}
			Data(const nlohmann::json& _wpJ) : 
				Data(_wpJ["id"], 
					static_cast<Access>(_wpJ["access"]), 
					_wpJ["globalAddr"], 
					static_cast<Condition>(_wpJ["cond"]), 
					_wpJ["value"], 
					static_cast<Type>(_wpJ["type"]), 
					_wpJ["len"], 
					_wpJ["active"]) 
			{};
		};
#pragma pack(pop)

		Watchpoint(Data&& _data, const std::string& _comment = "");

		void Update(Watchpoint&& _bp);

		auto Check(const Access _access, const GlobalAddr _globalAddr, const uint8_t _value) -> const bool;
		auto GetAccessI() const -> int;
		auto GetComment() const -> const std::string& { return comment;  };
		auto GetConditionS() const -> const char*;
		auto GetAccessS() const -> const char*;
		auto GetTypeS() const -> const char*;
		void Reset();
		void Print() const;
		auto GetJson() const -> nlohmann::json
		{
			return {
				{"id", data.id},
				{"access", static_cast<uint32_t>(data.access)},
				{"globalAddr", data.globalAddr},
				{"cond", static_cast<uint32_t>(data.cond)},
				{"value", data.value},
				{"type", static_cast<uint32_t>(data.type)},
				{"len", data.len},
				{"active", data.active},
				{"comment", comment}
			};
		};

		Data data;
		std::string comment;
	};
}