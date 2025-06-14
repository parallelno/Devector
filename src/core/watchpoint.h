#pragma once

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <format>

#include "utils/types.h"
#include "utils/str_utils.h"
#include "utils/utils.h"
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

		static auto GetAccess(const std::string _accessS) 
		-> Access
		{ 
			for (int i = 0; i < static_cast<int>(Access::COUNT); i++) 
			{ 
				if (std::string(wpAccessS[i]) == _accessS) 
				{ 
					return static_cast<Access>(i); 
				} 
			} 
			return Access::COUNT;
		}

		static auto GetType(const std::string _typeS)
		-> Type 
		{
			for (int i = 0; i < static_cast<int>(Type::COUNT); i++) 
			{ 
				if (std::string(wpTypesS[i]) == _typeS) 
				{ 
					return static_cast<Type>(i); 
				} 
			} 
			return Type::COUNT;
		}

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
					GetAccess(_wpJ["access"].get<std::string>()), 
					dev::StrHexToInt(_wpJ["globalAddr"].get<std::string>()), 
					GetCondition(_wpJ["cond"].get<std::string>()), 
					dev::StrHexToInt(_wpJ["value"].get<std::string>()),
					GetType(_wpJ["type"].get<std::string>()),
					dev::StrHexToInt(_wpJ["len"].get<std::string>()),
					_wpJ["active"])
			{};
		};
#pragma pack(pop)

		Watchpoint(Data&& _data, const std::string& _comment = "");

		void Update(Watchpoint&& _wp);

		auto Check(const Access _access, const GlobalAddr _globalAddr, const uint8_t _value) -> const bool;
		auto GetAccessI() const -> int;
		auto GetComment() const -> const std::string& { return comment;  };
		auto GetConditionS() const -> const char*;
		auto GetAccessS() const -> const char*;
		auto GetTypeS() const -> const char*;
		void Reset();
		void Print() const;
		auto ToJson() const -> nlohmann::json
		{
			return {
				{"id", data.id},
				{"access", GetAccessS()},
				{"globalAddr", std::format("0x{:06X}", data.globalAddr)},
				{"cond", GetConditionS()},
				{"value", std::format("0x{:04X}", data.value)},
				{"type", GetTypeS()},
				{"len", std::format("0x{:04X}", data.len)},
				{"active", data.active},
				{"comment", comment}
			};
		};

		Data data;
		std::string comment;
	};
}