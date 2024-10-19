#include <string>

#include "breakpoints.h"
#include "utils/str_utils.h"
#include "utils/utils.h"

void dev::Breakpoints::Clear()
{
	m_bps.clear();
	m_updates++;
}

void dev::Breakpoints::Reset()
{
	m_updates = 0;
}

void dev::Breakpoints::SetStatus(const Addr _addr, const Breakpoint::Status _status)
{
	m_updates++;
	auto bpI = m_bps.find(_addr);
	if (bpI != m_bps.end()) {
		bpI->second.data.structured.status = _status;
		return;
	}
	Add(Breakpoint{ _addr });
}

void dev::Breakpoints::Add(Breakpoint&& _bp )
{
	m_updates++;
	auto bpI = m_bps.find(_bp.data.structured.addr);
	if (bpI != m_bps.end())
	{
		bpI->second.Update(std::move(_bp));
		return;
	}

	m_bps.emplace(static_cast<Addr>(_bp.data.structured.addr), std::move(_bp));
}

void dev::Breakpoints::Del(const Addr _addr)
{
	m_updates++;
	auto bpI = m_bps.find(_addr);
	if (bpI != m_bps.end())
	{
		m_bps.erase(bpI);
	}
}

auto dev::Breakpoints::GetStatus(const Addr _addr)
-> const Breakpoint::Status
{
	auto bpI = m_bps.find(_addr);
	return bpI == m_bps.end() ? Breakpoint::Status::DELETED : bpI->second.data.structured.status;
}

bool dev::Breakpoints::Check(const CpuI8080::State& _cpuState, const Memory::State& _memState)
{
	auto bpI = m_bps.find(_cpuState.regs.pc.word);
	if (bpI == m_bps.end()) return false;

	auto status = bpI->second.CheckStatus(_cpuState, _memState);
	if (bpI->second.data.structured.autoDel)
	{
		m_bps.erase(bpI);
		m_updates++;
	}
	return status;
}

auto dev::Breakpoints::GetAll()
-> const BpMap&
{
	return m_bps;
}

auto dev::Breakpoints::GetUpdates()
-> const uint32_t
{
	return m_updates;
}