#pragma once
#ifndef DEV_WATCHPOINT_H
#define DEV_WATCHPOINT_H

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <format>

#include "Utils/Types.h"
#include "Core/I8080.h"
#include "Core/Memory.h"

namespace dev
{
	#define WATCHPOINT_CONDITIONS_LEN 7
	const std::string WATCHPOINT_CONDITION_STR[WATCHPOINT_CONDITIONS_LEN] = { "ANY", "=", "<", ">", "<=", ">=", "!="};
	static Id watchpointId = 0;

	class Watchpoint
	{
	public:
		enum class Type : int { LEN = 0, WORD }; // LEN - m_len bytes to check, WORD - one word to check
		enum class Access : int { R = 0, W, RW, COUNT };
		const std::string ACCESS_STR[3] = { "R-", "-W", "RW" };

		enum class Condition : int { ANY = 0, EQU, LESS, GREATER, LESS_EQU, GREATER_EQU, NOT_EQU, INVALID, COUNT };

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
		auto GetAccessS() const -> const std::string&;
		auto GetCondition() const -> Condition;
		auto GetConditionS() const -> const std::string&;
		auto GetValue() const -> uint16_t;
		auto GetType() const -> Type;
		auto GetLen() const -> int;
		auto GetComment() const -> const std::string&;
		auto GetId() const -> Id;
		static auto StrToCondition(const std::string& _condS) -> Watchpoint::Condition;
		void Reset();
		void Print() const;
		//auto operator=(const auto& _wp) const -> Watchpoint;
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
#endif // !DEV_WATCHPOINT_H