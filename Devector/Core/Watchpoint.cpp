#include "Watchpoint.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>
#include "Utils/StringUtils.h"
#include "Utils/Utils.h"

dev::Watchpoint::Watchpoint(const Access _access, const GlobalAddr _globalAddr, const Condition _cond,
			const uint16_t _value, const Type _type, const int _len,
			const bool _active, const std::string& _comment, 
			const bool _breakH, const bool _breakL)
	:
	m_access(_access), m_globalAddr(_globalAddr), m_cond(_cond),
	m_value(_value), m_type(_type), 
	m_len(_type == Type::LEN ? dev::Max(1, _len) : 2),
	m_active(_active), m_comment(_comment),
	m_breakH(_breakH), m_breakL(_breakL), m_id(watchpointId++)
{}

dev::Watchpoint::Watchpoint(const Watchpoint& _wp)
	:
	m_access(_wp.GetAccess()), m_globalAddr(_wp.GetGlobalAddr()), m_cond(_wp.GetCondition()),
	m_value(_wp.GetValue()), m_type(_wp.GetType()), m_len(_wp.GetLen()), m_active(_wp.IsActive()), 
	m_comment(_wp.GetComment()), m_id(_wp.GetId())
{}

void dev::Watchpoint::Update(const Access _access, const GlobalAddr _globalAddr,
	const Condition _cond, const uint16_t _value, const Type _type, const int _len,
	const bool _active, const std::string& _comment)
{
	m_access = _access;
	m_globalAddr = _globalAddr;
	m_cond = _cond;
	m_value = _value;
	m_type = _type;
	m_len = _len;
	m_active = _active;
	m_comment = _comment;
}

auto dev::Watchpoint::Check(const Access _access, const GlobalAddr _globalAddr, const uint8_t _value)
->const bool
{
	if (!m_active) return false;
	if (m_access != Access::RW && m_access != _access) return false;
	if (_globalAddr < m_globalAddr || _globalAddr >= m_globalAddr + m_len) return false;

	bool* break_p;
	uint8_t value_byte;

	if (m_type == Type::LEN)
	{
		break_p = &m_breakL;
		value_byte = m_value & 0xff;
	}
	else 
	// check the hi byte of a word
	if (m_globalAddr + 1 != _globalAddr) {
		return false;
	} 
	else{
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

	return m_breakL && (m_type == Type::LEN || m_breakH);
}

auto dev::Watchpoint::IsActive() const
-> bool
{
	return m_active;
}

auto dev::Watchpoint::GetGlobalAddr() const 
-> GlobalAddr
{
	return m_globalAddr;
}

auto dev::Watchpoint::GetAccess() const
-> Access
{
	return m_access;
}

auto dev::Watchpoint::GetAccessI() const 
-> int
{ 
	return static_cast<int>(m_access); 
}

auto dev::Watchpoint::GetAccessS() const
-> const std::string&
{
	return ACCESS_STR[static_cast<size_t>(m_access)];
}

auto dev::Watchpoint::GetCondition() const
-> Condition
{
	return m_cond;
}

auto dev::Watchpoint::GetConditionS() const
-> const std::string&
{
	return WATCHPOINT_CONDITION_STR[static_cast<size_t>(m_cond)];
}

auto dev::Watchpoint::GetValue() const
-> uint16_t
{
	return m_value;
}

auto dev::Watchpoint::GetType() const
-> Type
{
	return m_type;
}

auto dev::Watchpoint::GetLen() const
-> int
{
	return m_len;
}

auto dev::Watchpoint::GetComment() const
-> const std::string&
{
	return m_comment;
}

auto dev::Watchpoint::GetId() const 
-> Id
{
	return m_id;
}

auto dev::Watchpoint::StrToCondition(const std::string& _condS)
-> Watchpoint::Condition
{
	Watchpoint::Condition cond = Watchpoint::Condition::INVALID;
	if (_condS == "any") return Condition::ANY;
	if (_condS == "==") return Condition::EQU;

	for (int i = 0; i < WATCHPOINT_CONDITIONS_LEN; i++)
	{
		cond = _condS == WATCHPOINT_CONDITION_STR[i] ? static_cast<Watchpoint::Condition>(i) : cond;
	}
	return cond;
}

void dev::Watchpoint::Reset()
{
	m_breakL = false;
	m_breakH = false;
}

void dev::Watchpoint::Print() const
{
	std::printf("0x%05x, access: %s, cond: %s, value: 0x%04x, type: %d, len: %d, active: %d \n", 
		m_globalAddr, 
		ACCESS_STR[static_cast<int>(m_access)].c_str(),
		WATCHPOINT_CONDITION_STR[static_cast<int>(m_cond)].c_str(),
		m_value, static_cast<int>(m_type), m_len, m_active);
}
auto dev::Watchpoint::operator=(const dev::Watchpoint& _wp)
-> Watchpoint
{
	if (this != &_wp) {
		m_access = _wp.m_access;
		m_globalAddr = _wp.m_globalAddr;
		m_cond = _wp.m_cond;
		m_value = _wp.m_value;
		m_type = _wp.m_type;
		m_len = _wp.m_len;
		m_active = _wp.m_active;
		m_breakL = _wp.m_breakL;
		m_breakH = _wp.m_breakH;
		m_comment = _wp.m_comment;
	}
	return *this;
}