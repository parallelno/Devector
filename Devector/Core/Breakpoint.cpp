#include "Breakpoint.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>

#include "Utils/StrUtils.h"
#include "Utils/Utils.h"

dev::Breakpoint::Breakpoint(const Addr _addr,
	const MemPages _memPages,
	const Status _status, const bool _autoDel, 
	const Operand _op, const Condition _cond,
	const size_t _val, const std::string& _comment)
	:
	addr(_addr), memPages(_memPages), status(_status), autoDel(_autoDel), 
	operand(_op), cond(_cond), value(_val),
	comment(_comment)
{
	UpdateAddrMappingS();
}

void dev::Breakpoint::Update(const Addr _addr,
	const MemPages _memPages,
	const Status _status, const bool _autoDel, 
	const Operand _op, const Condition _cond,
	const size_t _val, const std::string& _comment)
{
	addr = _addr;
	memPages = _memPages;
	status = _status;
	autoDel = _autoDel;
	operand = _op;
	cond = _cond;
	value = _val;
	comment = _comment;
	UpdateAddrMappingS();
}

auto dev::Breakpoint::GetOperandS() const ->const char* { return bpOperandsS[static_cast<uint8_t>(operand)]; }
auto dev::Breakpoint::GetConditionS() const 
-> const std::string
{	
	std::string condValS = ConditionsS[static_cast<uint8_t>(cond)];
	condValS += cond == Condition::ANY ? "" : std::to_string(value);

	std::string out = std::format("{}{}{}", 
		GetOperandS(),
		condValS,
		autoDel ? ":A" : ""
	);
	return out;
}
auto dev::Breakpoint::IsActiveS() const -> const char* { return status == Status::ACTIVE ? "X" : "-"; }

bool dev::Breakpoint::CheckStatus(const CpuI8080::State& _cpuState, const Memory::State& _memState) const
{
	auto mapping = _memState.update.mapping.data & Memory::MAPPING_RAM_MODE_MASK ? 1 << (_memState.update.mapping.pageRam + 1 + 4 * _memState.update.ramdiskIdx) : 1;

	bool active = status == Status::ACTIVE && mapping & memPages.data;
	if (!active) return false;
	if (cond == dev::Condition::ANY) return true;
	
	uint64_t op;
	switch (operand)
	{
	case dev::Breakpoint::Operand::A:
		op = _cpuState.regs.psw.a;
		break;
	case dev::Breakpoint::Operand::F:
		op = _cpuState.regs.psw.af.l;
		break;
	case dev::Breakpoint::Operand::B:
		op = _cpuState.regs.bc.h;
		break;
	case dev::Breakpoint::Operand::C:
		op = _cpuState.regs.bc.l;
		break;
	case dev::Breakpoint::Operand::D:
		op = _cpuState.regs.de.h;
		break;
	case dev::Breakpoint::Operand::E:
		op = _cpuState.regs.de.l;
		break;
	case dev::Breakpoint::Operand::H:
		op = _cpuState.regs.hl.h;
		break;
	case dev::Breakpoint::Operand::L:
		op = _cpuState.regs.hl.l;
		break;
	case dev::Breakpoint::Operand::PSW:
		op = _cpuState.regs.psw.af.word;
		break;
	case dev::Breakpoint::Operand::BC:
		op = _cpuState.regs.bc.word;
		break;
	case dev::Breakpoint::Operand::DE:
		op = _cpuState.regs.de.word;
		break;
	case dev::Breakpoint::Operand::HL:
		op = _cpuState.regs.hl.word;
		break;
	case dev::Breakpoint::Operand::CC:
		op = _cpuState.cc;
		break;
	case dev::Breakpoint::Operand::SP:
		op = _cpuState.regs.sp.word;
		break;
	default:
		op = 0;
		break;
	}

	switch (cond)
	{
	case dev::Condition::EQU:
		return op == value;
	case dev::Condition::LESS:
		return op < value;
	case dev::Condition::GREATER:
		return op > value;
	case dev::Condition::LESS_EQU:
		return op <= value;
	case dev::Condition::GREATER_EQU:
		return op >= value;
	case dev::Condition::NOT_EQU:
		return op != value;
	}
	return false;
}

void dev::Breakpoint::Print() const
{
	dev::Log("0x{:04x}, status:{}, memPages: {}, autoDel: {}, op: {}, cond: {}, val: {}",
		addr, static_cast<int>(status), memPages.data, autoDel,
		GetOperandS(), GetConditionS(), value);
}

void dev::Breakpoint::UpdateAddrMappingS()
{
	addrMappingS = std::format(
		"0x{:04X} {} 1:{}{}{}{}"
		" 2:{}{}{}{}"
		" 3:{}{}{}{}"
		" 4:{}{}{}{}"
		" 5:{}{}{}{}"
		" 6:{}{}{}{}"
		" 7:{}{}{}{}"
		" 8:{}{}{}{}",
		addr,
		memPages.ram ? "M" : "_",
		memPages.rdisk0page0 ? "0" : "_",
		memPages.rdisk0page1 ? "1" : "_",
		memPages.rdisk0page2 ? "2" : "_",
		memPages.rdisk0page3 ? "3" : "_",
		memPages.rdisk1page0 ? "0" : "_",
		memPages.rdisk1page1 ? "1" : "_",
		memPages.rdisk1page2 ? "2" : "_",
		memPages.rdisk1page3 ? "3" : "_",
		memPages.rdisk2page0 ? "0" : "_",
		memPages.rdisk2page1 ? "1" : "_",
		memPages.rdisk2page2 ? "2" : "_",
		memPages.rdisk2page3 ? "3" : "_",
		memPages.rdisk3page0 ? "0" : "_",
		memPages.rdisk3page1 ? "1" : "_",
		memPages.rdisk3page2 ? "2" : "_",
		memPages.rdisk3page3 ? "3" : "_",
		memPages.rdisk4page0 ? "0" : "_",
		memPages.rdisk4page1 ? "1" : "_",
		memPages.rdisk4page2 ? "2" : "_",
		memPages.rdisk4page3 ? "3" : "_",
		memPages.rdisk5page0 ? "0" : "_",
		memPages.rdisk5page1 ? "1" : "_",
		memPages.rdisk5page2 ? "2" : "_",
		memPages.rdisk5page3 ? "3" : "_",
		memPages.rdisk6page0 ? "0" : "_",
		memPages.rdisk6page1 ? "1" : "_",
		memPages.rdisk6page2 ? "2" : "_",
		memPages.rdisk6page3 ? "3" : "_",
		memPages.rdisk7page0 ? "0" : "_",
		memPages.rdisk7page1 ? "1" : "_",
		memPages.rdisk7page2 ? "2" : "_",
		memPages.rdisk7page3 ? "3" : "_"
		);
}

auto dev::Breakpoint::GetAddrMappingS() const
-> const char*
{ 
	return addrMappingS.c_str();
};