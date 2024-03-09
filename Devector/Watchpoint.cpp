#include "Watchpoint.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>
#include "Utils/StringUtils.h"
#include "Utils/Utils.h"

auto dev::Watchpoint::IsActive() const
->const bool
{
	return m_active;
}

auto dev::Watchpoint::Check(const Access _access, const GlobalAddr _globalAddr, const uint8_t _value)
->const bool
{
	if (!m_active) return false;
	if (m_access != Access::RW && m_access != _access) return false;

	if (m_breakL && (m_valueSize == VAL_BYTE_SIZE || m_breakH)) return true;

	bool* break_p;
	uint8_t value_byte;

	if (_globalAddr == m_globalAddr)
	{
		break_p = &m_breakL;
		value_byte = m_value & 0xff;
	}
	else
	{
		break_p = &m_breakH;
		value_byte = m_value >> 8 & 0xff;
	}

	switch (m_cond)
	{
	case Condition::ANY:
		*break_p = true;
		break;
	case Condition::EQU:
		*break_p = _value == value_byte;
		break;
	case Condition::LESS:
		*break_p = _value < value_byte;
		break;
	case Condition::GREATER:
		*break_p = _value > value_byte;
		break;
	case Condition::LESS_EQU:
		*break_p = _value <= value_byte;
		break;
	case Condition::GREATER_EQU:
		*break_p = _value >= value_byte;
		break;
	case Condition::NOT_EQU:
		*break_p = _value != value_byte;
		break;
	default:
		return false;
	};

	return m_breakL && (m_valueSize == VAL_BYTE_SIZE || m_breakH);
}

auto dev::Watchpoint::GetGlobalAddr() const
-> const size_t
{
	return m_globalAddr;
}

auto dev::Watchpoint::CheckAddr(const GlobalAddr _globalAddr) const
-> const bool
{
	return (_globalAddr == m_globalAddr) || ((_globalAddr == m_globalAddr + 1) && (m_valueSize == VAL_WORD_SIZE));
}

void dev::Watchpoint::Reset()
{
	m_breakL = false;
	m_breakH = false;
}

void dev::Watchpoint::Print() const
{
	std::printf("0x%05x, access: %s, cond: %s, value: 0x%04x, value_size: %zd, active: %d \n", 
		m_globalAddr, 
		access_s[static_cast<size_t>(m_access)], 
		conditions_s[static_cast<size_t>(m_cond)], 
		m_value, m_valueSize, m_active);
}

auto dev::Watchpoint::operator=(const auto& _wp) const
-> Watchpoint
{
	// TODO: check if it correctly copies
	return { m_access, m_globalAddr, m_cond,
		m_value, m_valueSize, m_active, m_breakH, m_breakL };
}
