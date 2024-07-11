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
{}

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
	auto mapping = _memState.mapping0.data & Memory::MAPPING_RAM_MODE_MASK ? 1 << (_memState.mapping0.pageRam + 1) :
				_memState.mapping1.data & Memory::MAPPING_RAM_MODE_MASK ? 1 << (_memState.mapping1.pageRam + 5) : 1;

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

auto dev::Breakpoint::GetAddrMappingS() const
-> const char*
{ 
	static char out[] = "0xFFFF M D0:0123 D1:0123";

	sprintf_s(out, 25, "0x%04X %s D0:%s%s%s%s D1:%s%s%s%s", addr,
		memPages.ram ? "M" : "_",
		memPages.rd00 ? "0" : "_",
		memPages.rd01 ? "1" : "_",
		memPages.rd02 ? "2" : "_",
		memPages.rd03 ? "3" : "_",
		memPages.rd10 ? "0" : "_",
		memPages.rd11 ? "1" : "_",
		memPages.rd12 ? "2" : "_",
		memPages.rd13 ? "3" : "_"
		);

	return out;
};