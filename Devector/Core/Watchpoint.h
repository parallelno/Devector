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
	static Id watchpointId = 0;
	static const char* wpAccessS[] = { "R", "W", "RW" };
	static const char* wpTypesS[] = { "LEN", "WORD" };

	class Watchpoint
	{
	public:
		// LEN - breaks if the condition succeds for any bytes in m_len range
		// WORD - breaks if the condition succeds for a word
		enum class Type : uint8_t { LEN = 0, WORD };
		enum class Access : uint8_t { R = 0, W, RW, COUNT };

		Watchpoint(const Access _access, const GlobalAddr _globalAddr, const Condition _cond,
			const uint16_t _value, const Type _type = Type::LEN, const int _len = 1,
			const bool _active = true, const std::string& _comment = "", 
			const bool _breakH = false, const bool _breakL = false);
		Watchpoint(const Watchpoint& _wp);

		void Update(const Access _access, const GlobalAddr _globalAddr, const Condition _cond,
			const uint16_t _value, const Type _type = Type::LEN, const int _len = 1,
			const bool _active = true, const std::string& _comment = "");
		auto Check(const Access _access, const GlobalAddr _globalAddr, const uint8_t _value) -> const bool;
		auto IsActive() const -> bool;
		auto GetGlobalAddr() const -> GlobalAddr;
		auto GetAccess() const -> Access;
		auto GetAccessI() const -> int;
		auto GetCondition() const -> Condition;
		auto GetValue() const -> uint16_t;
		auto GetType() const -> Type;
		auto GetLen() const -> int;
		auto GetComment() const -> const std::string&;
		auto GetConditionS() const -> const char*;
		auto GetAccessS() const -> const char*;
		auto GetTypeS() const -> const char*;
		auto GetId() const -> Id;
		void Reset();
		void Print() const;
		auto operator=(const dev::Watchpoint& _wp)->Watchpoint;

	private:
		Id m_id;
		Access m_access;
		GlobalAddr m_globalAddr;
		Condition m_cond;
		uint16_t m_value;
		Type m_type;
		int m_len;
		bool m_active;
		bool m_breakL;
		bool m_breakH;
		std::string m_comment;
	};
}