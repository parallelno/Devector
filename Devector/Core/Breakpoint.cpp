#include "Breakpoint.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>

#include "Utils/StrUtils.h"
#include "Utils/Utils.h"

dev::Breakpoint::Breakpoint(const Addr _addr,
	const uint8_t _mappingPages,
	const Status _status, const bool _autoDel, 
	const Operand _op, const Condition _cond,
	const size_t _val, const std::string& _comment)
	:
	addr(_addr), mappingPages(_mappingPages), status(_status), autoDel(_autoDel), 
	operand(_op), cond(_cond), value(_val),
	comment(_comment)
{}

void dev::Breakpoint::Update(const Addr _addr,
	const uint8_t _mappingPages,
	const Status _status, const bool _autoDel, 
	const Operand _op, const Condition _cond,
	const size_t _val, const std::string& _comment)
{
	addr = _addr;
	mappingPages = _mappingPages;
	status = _status;
	autoDel = _autoDel;
	operand = _op;
	cond = _cond;
	value = _val;
	comment = _comment;
}

auto dev::Breakpoint::GetOperandS() const ->const char* { return bpOperandsS[static_cast<uint8_t>(operand)]; }
auto dev::Breakpoint::GetConditionS() const ->const char* {	return bpCondsS[static_cast<uint8_t>(operand)]; }
auto dev::Breakpoint::IsActiveS() const -> const char* { return status == Status::ACTIVE ? "X" : "-"; }

bool dev::Breakpoint::CheckStatus(const CpuI8080::State& _cpuState, const Memory::State& _memState) const
{
	auto mapping = _memState.mapping.data && Memory::MAPPING_RAM_MODE_MASK ? 1 << (_memState.mapping.pageRam + 1) : 1;
	bool active = status == Status::ACTIVE && mapping & mappingPages;
	if (!active) return false;
	if (cond == dev::Breakpoint::Condition::ANY) return true;
	
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
	case dev::Breakpoint::Condition::EQU:
		return op == value;
	case dev::Breakpoint::Condition::LESS:
		return op < value;
	case dev::Breakpoint::Condition::GREATER:
		return op > value;
	case dev::Breakpoint::Condition::LESS_EQU:
		return op <= value;
	case dev::Breakpoint::Condition::GREATER_EQU:
		return op >= value;
	case dev::Breakpoint::Condition::NOT_EQU:
		return op != value;
	}
	return false;
}

void dev::Breakpoint::Print() const
{
	dev::Log("0x{:04x}, status:{}, mappingPages: {}, autoDel: {}, op: {}, cond: {}, val: {}",
		addr, static_cast<int>(status), mappingPages, autoDel,
		GetOperandS(), GetConditionS(), value);
}

auto dev::Breakpoint::GetAddrMappingS() const
-> const char*
{ 
	static char out[] = "0xFFFF M01234";

	sprintf_s(out + 2, 5, "%04X", addr);

	out[6] = ' ';
	if (mappingPages & Breakpoint::MAPPING_RAM)			  out[7]  = 'M';
	if (mappingPages & Breakpoint::MAPPING_RAMDISK_PAGE0) out[8]  = '0';
	if (mappingPages & Breakpoint::MAPPING_RAMDISK_PAGE1) out[9]  = '1';
	if (mappingPages & Breakpoint::MAPPING_RAMDISK_PAGE2) out[10] = '2';
	if (mappingPages & Breakpoint::MAPPING_RAMDISK_PAGE3) out[11] = '3';
	out[12] = 0;

	return out;
};