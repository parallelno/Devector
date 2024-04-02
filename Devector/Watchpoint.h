#pragma once
#ifndef DEV_WATCHPOINT_H
#define DEV_WATCHPOINT_H

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <format>

#include "Types.h"
#include "I8080.h"
#include "Memory.h"

namespace dev
{
	#define WATCHPOINT_CONDITIONS_LEN 7
	const std::string WATCHPOINT_CONDITION_STR[WATCHPOINT_CONDITIONS_LEN] = { "ANY", "==", "<", ">", "<=", ">=", "!=" };
	static size_t watchpointId = 0;

	class Watchpoint
	{
	public:
		using Id = size_t;
		enum class Access : size_t { R = 0, W, RW, COUNT };
		const std::string ACCESS_STR[3] = { "R-", "-W", "RW" };
		static constexpr size_t VAL_BYTE_SIZE = sizeof(uint8_t);
		static constexpr size_t VAL_WORD_SIZE = sizeof(uint16_t);
		static constexpr size_t VAL_MAX_SIZE = VAL_WORD_SIZE;

		enum class Condition : size_t { ANY = 0, EQU, LESS, GREATER, LESS_EQU, GREATER_EQU, NOT_EQU, INVALID, COUNT };

		Watchpoint(const Access _access, const GlobalAddr _globalAddr, const Condition _cond,
			const uint16_t _value, const size_t _size = VAL_BYTE_SIZE,
			const bool _active = true, const std::string& _comment = "", 
			const bool _breakH = false, const bool _breakL = false);
		Watchpoint(const Watchpoint& _wp);

		void Update(const Access _access, const GlobalAddr _globalAddr, const Condition _cond,
			const uint16_t _value, const size_t _size = VAL_BYTE_SIZE,
			const bool _active = true, const std::string& _comment = "");
		auto Check(const Access _access, const GlobalAddr _globalAddr, const uint8_t _value) -> const bool;
		auto IsActive() const -> const bool;
		auto GetGlobalAddr() const -> const GlobalAddr;
		auto GetAccess() const -> const Access;
		auto GetAccessS() const -> const std::string&;
		auto GetCondition() const -> const Condition;
		auto GetConditionS() const -> const std::string&;
		auto GetValue() const -> const uint16_t;
		auto GetSize() const -> const size_t;
		auto GetSizeS() const -> const char*;
		auto GetComment() const -> const std::string&;
		auto GetId() const -> Id;
		auto CheckAddr(const GlobalAddr _globalAddr) const -> const bool;
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
		size_t m_size; // how many bytes to check
		bool m_active;
		bool m_breakL;
		bool m_breakH;
		std::string m_comment;
	};
}
#endif // !DEV_WATCHPOINT_H