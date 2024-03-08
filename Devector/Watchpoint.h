#pragma once
#ifndef DEV_WATCHPOINT_H
#define DEV_WATCHPOINT_H

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <format>

#include "I8080.h"
#include "Memory.h"

namespace dev
{
	class Watchpoint
	{
	public:
		enum class Access : size_t { R = 0, W, RW, COUNT };
		static constexpr const char* access_s[] = { "R-", "-W", "RW" };
		static constexpr size_t VAL_BYTE_SIZE = sizeof(uint8_t);
		static constexpr size_t VAL_WORD_SIZE = sizeof(uint16_t);
		static constexpr size_t VAL_MAX_SIZE = VAL_WORD_SIZE;

		enum class Condition : size_t { ANY = 0, EQU, LESS, GREATER, LESS_EQU, GREATER_EQU, NOT_EQU, COUNT };
		static constexpr const char* conditions_s[] = { "ANY", "==", "<", ">", "<=", ">=", "!=" };

		Watchpoint(const Access _access, const uint32_t _globalAddr, const Condition _cond, const uint16_t _value, const size_t _valueSize = VAL_BYTE_SIZE, const bool _active = true, const bool _breakH = false, const bool _breakL = false)
			: m_access(static_cast<Access>((size_t)_access % (size_t)Access::COUNT)), m_globalAddr(_globalAddr), m_cond(static_cast<Condition>((size_t)_cond& (size_t)Condition::COUNT)),
			m_value(_value & 0xffff), m_valueSize(_valueSize), m_active(_active), m_breakH(_breakH), m_breakL(_breakL)
		{}
		auto Check(const Access _access, const uint32_t _globalAddr, const uint8_t _value) -> const bool;
		auto IsActive() const -> const bool;
		auto GetGlobalAddr() const -> const size_t;
		auto CheckAddr(const uint32_t _globalAddr) const -> const bool;
		void Reset();
		void Print() const;
		auto operator=(const auto& _wp) const -> Watchpoint;

	private:
		Access m_access;
		uint32_t m_globalAddr;
		Condition m_cond;
		uint16_t m_value;
		size_t m_valueSize;
		bool m_active;
		bool m_breakL;
		bool m_breakH;
	};
}
#endif // !DEV_WATCHPOINT_H