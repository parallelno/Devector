#include "Breakpoints.h"
#include <string>

#include "Utils/StrUtils.h"
#include "Utils/Utils.h"

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
		bpI->second.data.status = _status;
		return;
	}
	Add(Breakpoint{ _addr });
}

void dev::Breakpoints::Add(Breakpoint&& _bp )
{
	m_updates++;
	auto bpI = m_bps.find(_bp.data.addr);
	if (bpI != m_bps.end())
	{
		bpI->second.Update(std::move(_bp));
		return;
	}

	m_bps.emplace(static_cast<Addr>(_bp.data.addr), std::move(_bp));
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

// UI thread
auto dev::Breakpoints::GetStatus(const Addr _addr)
-> const Breakpoint::Status
{
	auto bpI = m_bps.find(_addr);
	return bpI == m_bps.end() ? Breakpoint::Status::DELETED : bpI->second.data.status;
}

// Hardware thread
bool dev::Breakpoints::Check(const CpuI8080::State& _cpuState, const Memory::State& _memState)
{
	auto bpI = m_bps.find(_cpuState.regs.pc.word);
	if (bpI == m_bps.end()) return false;

	auto status = bpI->second.CheckStatus(_cpuState, _memState);
	if (bpI->second.data.autoDel)
	{
		m_bps.erase(bpI);
		m_updates++;
	}
	return status;
}

// UI thread
auto dev::Breakpoints::GetAll()
-> const BpMap
{
	BpMap out;
	for (const auto& [addr, bp] : m_bps)
	{
		out.insert({ addr, bp });
	}
	return out;
}

auto dev::Breakpoints::GetUpdates()
-> const uint32_t
{
	return m_updates;
}