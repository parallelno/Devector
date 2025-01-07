
#include <string>

#include "core/watchpoints.h"
#include "utils/str_utils.h"
#include "utils/utils.h"

// Hardware thread
void dev::Watchpoints::Clear()
{
	m_wps.clear();
	m_updates++;
}

// Hardware thread
void dev::Watchpoints::Add(Watchpoint&& _wp)
{
	m_updates++;

	auto wpI = m_wps.find(_wp.data.id);
	if (wpI != m_wps.end())
	{
		wpI->second.Update(std::move(_wp));
		return;
	}
	
	m_wps.emplace(_wp.data.id, std::move(_wp));
}

void dev::Watchpoints::Add(const nlohmann::json& _wpJ)
{
	m_updates++;

	Watchpoint::Data wpData {_wpJ};
	Watchpoint wp{ std::move(wpData), _wpJ["comment"] };

	auto wpI = m_wps.find(wp.data.id);
	if (wpI != m_wps.end())
	{
		wpI->second.Update(std::move(wp));
		return;
	}
	
	m_wps.emplace(wp.data.id, std::move(wp));
}

// Hardware thread
void dev::Watchpoints::Del(const dev::Id _id)
{
	m_updates++;
	auto bpI = m_wps.find(_id);
	if (bpI != m_wps.end())
	{
		m_wps.erase(bpI);
	}
}

// Hardware thread
void dev::Watchpoints::Check(const Watchpoint::Access _access, const GlobalAddr _globalAddr, const uint8_t _value)
{
	auto wpI = std::find_if(m_wps.begin(), m_wps.end(),
		[_access, _globalAddr, _value](WpMap::value_type& pair)
		{
			return pair.second.Check(_access, _globalAddr, _value);
		});
	
	m_wpBreak |= wpI != m_wps.end();
}

// Hardware thread
auto dev::Watchpoints::GetAll()
-> const WpMap&
{
	return m_wps;
}

// Hardware thread
auto dev::Watchpoints::GetUpdates()
-> const uint32_t
{
	return m_updates;
}

bool dev::Watchpoints::CheckBreak()
{
	if (!m_wpBreak) return false;

	m_wpBreak = false;

	// reset wps break status
	for (auto& [id, watchpoint] : m_wps)
	{
		watchpoint.Reset();
	}

	return true;
}