#include "Watchpoint.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>
#include "Utils/StrUtils.h"
#include "Utils/Utils.h"

dev::Watchpoint::Watchpoint(Data&& _data, const std::string& _comment)
	:
	data(std::move(_data)), comment(_comment)
{
	//UpdateAddrMappingS();
}

void dev::Watchpoint::Update(Watchpoint&& _bp)
{
	data = std::move(_bp.data);
	comment = std::move(_bp.comment);
	//UpdateAddrMappingS();
}

auto dev::Watchpoint::Check(const Access _access, const GlobalAddr _globalAddr, const uint8_t _value)
->const bool
{
	if (!data.active) return false;
	if (data.access != Access::RW && data.access != _access) return false;
	if (_globalAddr < data.globalAddr || _globalAddr >= data.globalAddr + data.len) return false;

	bool break_;
	bool low = true;

	uint8_t value_byte;

	if (data.type == Type::LEN)
	{
		value_byte = data.value & 0xff;
	}
	else // check the hi byte of a word 
	if (data.globalAddr + 1 != _globalAddr) {
		return false;
	} 
	else{
		low = false;
		value_byte = data.value >> 8 & 0xff;
	}

	switch (data.cond)
	{
	case Condition::ANY:
		break_ = true;
		break;
	case Condition::EQU:
		break_ = _value == value_byte;
		break;
	case Condition::LESS:
		break_ = _value < value_byte;
		break;
	case Condition::GREATER:
		break_ = _value > value_byte;
		break;
	case Condition::LESS_EQU:
		break_ = _value <= value_byte;
		break;
	case Condition::GREATER_EQU:
		break_ = _value >= value_byte;
		break;
	case Condition::NOT_EQU:
		break_ = _value != value_byte;
		break;
	default:
		return false;
	};

	if (low) {
		data.breakL = break_;
	}
	else {
		data.breakH = break_;
	}

	return data.breakL && (data.type == Type::LEN || data.breakH);
}

auto dev::Watchpoint::GetAccessI() const -> int { return static_cast<int>(data.access); }
auto dev::Watchpoint::GetAccessS() const -> const char* { return wpAccessS[static_cast<uint8_t>(data.access)]; }
auto dev::Watchpoint::GetConditionS() const -> const char* { return ConditionsS[static_cast<uint8_t>(data.cond)];}
auto dev::Watchpoint::GetTypeS() const -> const char* {	return wpTypesS[static_cast<uint8_t>(data.type)]; }

void dev::Watchpoint::Reset()
{
	data.breakL = false;
	data.breakH = false;
}

void dev::Watchpoint::Print() const
{
	std::printf("0x%05x, access: %s, cond: %s, value: 0x%04x, type: %s, len: %d, active: %d \n", 
		data.globalAddr, GetAccessS(), GetConditionS(),
		data.value, GetTypeS(), data.len, data.active);
}