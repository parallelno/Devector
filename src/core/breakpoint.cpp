#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>

#include "core/breakpoint.h"
#include "utils/str_utils.h"
#include "utils/utils.h"

dev::Breakpoint::Breakpoint(Data&& _data, const std::string& _comment)
	:
	data(std::move(_data)), comment(_comment)
{
	UpdateAddrMappingS();
}

void dev::Breakpoint::Update(Breakpoint&& _bp)
{
	data = std::move(_bp.data);
	comment = std::move(_bp.comment);
	UpdateAddrMappingS();
}

auto dev::Breakpoint::GetOperandS() const 
-> const char* 
{ 
	return bpOperandsS[static_cast<uint8_t>(data.structured.operand)]; 
}

auto dev::Breakpoint::GetConditionS() const 
-> const std::string
{	
	std::string condValS = ConditionsS[static_cast<uint8_t>(data.structured.cond)];
	condValS += data.structured.cond == Condition::ANY ? "" : std::format(" 0x{:02X}", data.structured.value);

	std::string out = std::format("{}{}{}", 
		GetOperandS(),
		condValS,
		data.structured.autoDel ? ":A" : ""
	);
	return out;
}

auto dev::Breakpoint::IsActiveS() const -> const char* { return data.structured.status == Status::ACTIVE ? "X" : "-"; }

bool dev::Breakpoint::CheckStatus(const CpuI8080::State& _cpuState, const Memory::State& _memState) const
{
	auto mapping = _memState.update.mapping.data & Memory::MAPPING_RAM_MODE_MASK ? 1 << (_memState.update.mapping.pageRam + 1 + 4 * _memState.update.ramdiskIdx) : 1;

	bool active = data.structured.status == Status::ACTIVE && mapping & data.structured.memPages.data;
	if (!active) return false;
	if (data.structured.cond == dev::Condition::ANY) return true;
	
	uint64_t op;
	switch (data.structured.operand)
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

	switch (data.structured.cond)
	{
	case dev::Condition::EQU:
		return op == data.structured.value;
	case dev::Condition::LESS:
		return op < data.structured.value;
	case dev::Condition::GREATER:
		return op > data.structured.value;
	case dev::Condition::LESS_EQU:
		return op <= data.structured.value;
	case dev::Condition::GREATER_EQU:
		return op >= data.structured.value;
	case dev::Condition::NOT_EQU:
		return op != data.structured.value;
	}
	return false;
}

void dev::Breakpoint::Print() const
{
	dev::Log("0x{:04X}, status:{}, memPages: {}, autoDel: {}, op: {}, cond: {}, val: {}",
		data.structured.addr, static_cast<int>(data.structured.status), data.structured.memPages.data, data.structured.autoDel,
		GetOperandS(), GetConditionS(), data.structured.value);
}

void dev::Breakpoint::UpdateAddrMappingS()
{
	addrMappingS = dev::Uint16ToStrC0x(data.structured.addr);
	/*
	addrMappingS = std::format(
		"0x{:04X} {}"
		" 1:{}{}{}{}"
		" 2:{}{}{}{}"
		" 3:{}{}{}{}"
		" 4:{}{}{}{}"
		" 5:{}{}{}{}"
		" 6:{}{}{}{}"
		" 7:{}{}{}{}"
		" 8:{}{}{}{}",
		static_cast<int>(data.structured.addr),
		data.structured.memPages.ram ? "M" : "_",
		data.structured.memPages.rdisk0page0 ? "0" : "_",
		data.structured.memPages.rdisk0page1 ? "1" : "_",
		data.structured.memPages.rdisk0page2 ? "2" : "_",
		data.structured.memPages.rdisk0page3 ? "3" : "_",
		data.structured.memPages.rdisk1page0 ? "0" : "_",
		data.structured.memPages.rdisk1page1 ? "1" : "_",
		data.structured.memPages.rdisk1page2 ? "2" : "_",
		data.structured.memPages.rdisk1page3 ? "3" : "_",
		data.structured.memPages.rdisk2page0 ? "0" : "_",
		data.structured.memPages.rdisk2page1 ? "1" : "_",
		data.structured.memPages.rdisk2page2 ? "2" : "_",
		data.structured.memPages.rdisk2page3 ? "3" : "_",
		data.structured.memPages.rdisk3page0 ? "0" : "_",
		data.structured.memPages.rdisk3page1 ? "1" : "_",
		data.structured.memPages.rdisk3page2 ? "2" : "_",
		data.structured.memPages.rdisk3page3 ? "3" : "_",
		data.structured.memPages.rdisk4page0 ? "0" : "_",
		data.structured.memPages.rdisk4page1 ? "1" : "_",
		data.structured.memPages.rdisk4page2 ? "2" : "_",
		data.structured.memPages.rdisk4page3 ? "3" : "_",
		data.structured.memPages.rdisk5page0 ? "0" : "_",
		data.structured.memPages.rdisk5page1 ? "1" : "_",
		data.structured.memPages.rdisk5page2 ? "2" : "_",
		data.structured.memPages.rdisk5page3 ? "3" : "_",
		data.structured.memPages.rdisk6page0 ? "0" : "_",
		data.structured.memPages.rdisk6page1 ? "1" : "_",
		data.structured.memPages.rdisk6page2 ? "2" : "_",
		data.structured.memPages.rdisk6page3 ? "3" : "_",
		data.structured.memPages.rdisk7page0 ? "0" : "_",
		data.structured.memPages.rdisk7page1 ? "1" : "_",
		data.structured.memPages.rdisk7page2 ? "2" : "_",
		data.structured.memPages.rdisk7page3 ? "3" : "_"
		);
		*/
}

auto dev::Breakpoint::GetAddrMappingS() const
-> const char*
{ 
	return addrMappingS.c_str();
};