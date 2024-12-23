#include <format>

#include "debug_data.h"

dev::DebugData::DebugData(Hardware& _hardware)
	: m_hardware(_hardware)
{}

void dev::DebugData::Reset()
{
	m_debugPath.clear();
	m_labels.clear();
	m_consts.clear();
	m_comments.clear();
	
	m_breakpoints.Clear();
	m_watchpoints.Clear();
}

bool IsConstLabel(const char* _s)
{
	// Iterate through each character in the string
	while (*_s != '\0') {
		// Check if the character is uppercase or underscore
		if (!(std::isupper(*_s) || *_s == '_' || (*_s >= '0' && *_s <= '9'))) {
			return false; // Not all characters are capital letters or underscores
		}
		++_s; // Move to the next character
	}
	return true; // All characters are capital letters or underscores
}

auto dev::DebugData::GetComment(const Addr _addr) const
-> const std::string*
{
	auto commentI = m_comments.find(_addr);
	return commentI != m_comments.end() ? &commentI->second : nullptr;
}

void dev::DebugData::SetComment(const Addr _addr, const std::string& _comment)
{
	m_comments[_addr] = _comment;
}

void dev::DebugData::DelComment(const Addr _addr)
{
	auto commentI = m_comments.find(_addr);
	m_comments.erase(commentI);
}

auto dev::DebugData::GetLabels(const Addr _addr) const -> const AddrLabels*
{
	auto labelsI = m_labels.find(_addr);
	return labelsI != m_labels.end() ? &labelsI->second : nullptr;
}

void dev::DebugData::SetLabels(const Addr _addr, const AddrLabels& _labels)
{
	if (_labels.empty()) {
		auto labelsI = m_labels.find(_addr);
		m_labels.erase(labelsI);
	}
	else {
		m_labels[_addr] = _labels;
	}
}

auto dev::DebugData::GetConsts(const Addr _addr) const -> const AddrLabels*
{
	auto constsI = m_consts.find(_addr);
	return constsI != m_consts.end() ? &constsI->second : nullptr;
}

void dev::DebugData::SetConsts(const Addr _addr, const AddrLabels& _consts)
{
	if (_consts.empty()) {
		auto constsI = m_consts.find(_addr);
		m_consts.erase(constsI);
	}
	else {
		m_consts[_addr] = _consts;
	}
	SaveDebugData();
}

void dev::DebugData::LoadDebugData(const std::wstring& _romPath)
{
	// check if the file exists
	auto romDir = dev::GetDir(_romPath);
	m_debugPath = romDir + L"\\" + dev::GetFilename(_romPath) + L".json";

	// init empty dictionaries when there is no file found
	if (!dev::IsFileExist(m_debugPath)) return;
	
	auto debugDataJ = LoadJson(m_debugPath);

	// add labels
	m_labels.clear();
	if (debugDataJ.contains("labels")) {
		for (auto& [str, addrS] : debugDataJ["labels"].items())
		{
			Addr addr = dev::StrHexToInt(addrS.get<std::string>().c_str());
			m_labels.emplace(addr, AddrLabels{}).first->second.emplace_back(str);
		}
	}
	// add consts
	m_consts.clear();
	if (debugDataJ.contains("consts")) {
		for (auto& [str, addrS] : debugDataJ["consts"].items())
		{
			Addr addr = dev::StrHexToInt(addrS.get<std::string>().c_str());
			m_consts.emplace(addr, AddrLabels{}).first->second.emplace_back(str);
		}
	}
	// add comments
	m_comments.clear();
	if (debugDataJ.contains("comments")) {
		for (auto& [addrS, str] : debugDataJ["comments"].items())
		{
			Addr addr = dev::StrHexToInt(addrS.c_str());
			m_comments.emplace(addr, str);
		}

	}
	// add breakpoints
	m_breakpoints.Clear();	
	if (debugDataJ.contains("breakpoints")) {
		for (auto& breakpointJ : debugDataJ["breakpoints"])
		{
			Breakpoint::Data bpData{ breakpointJ };

			Breakpoint bp{ std::move(bpData), breakpointJ["comment"] };
			auto addr = bp.data.structured.addr;
			m_breakpoints.Add(std::move(bp));
		}
	}
	// add watchpoints
	m_watchpoints.Clear();	
	if (debugDataJ.contains("watchpoints")) {
		for (auto& watchpointJ : debugDataJ["watchpoints"])
		{
			Watchpoint::Data wpData{ watchpointJ };

			Watchpoint wp{ std::move(wpData), watchpointJ["comment"] };
			m_watchpoints.Add(std::move(wp));
		}
	}
}

void dev::DebugData::SaveDebugData()
{
	if (m_debugPath.empty()) return;

	nlohmann::json debugDataJ = {};

	bool empty = true;
	
	// update labels
	empty &= m_labels.empty();
	debugDataJ["labels"] = {};
	auto& debugLabels = debugDataJ["labels"];
	for (const auto& [addr, labels] : m_labels)
	{
		for (int i = 0; i < labels.size(); i++) {
			debugLabels[labels[i]] = std::format("0x{:04X}", addr);
		}
	}

	// update consts
	empty &= m_consts.empty();
	debugDataJ["consts"] = {};
	auto& debugConsts = debugDataJ["consts"];
	for (const auto& [addr, consts] : m_consts)
	{
		for (int i = 0; i < consts.size(); i++) {
			debugConsts[consts[i]] = std::format("0x{:04X}", addr);
		}
	}

	// update comments
	empty &= m_comments.empty();
	debugDataJ["comments"] = {};
	auto& debugComments = debugDataJ["comments"];
	for (const auto& [addr, comment] : m_comments)
	{
		debugComments[std::format("0x{:04X}", addr)] = comment;
	}


	// update breakpoints
	empty &= m_breakpoints.GetAll().empty();
	debugDataJ["breakpoints"] = {};
	auto& debugBreakpoints = debugDataJ["breakpoints"];
	for (const auto& [addr, bp] : m_breakpoints.GetAll())
	{
		debugBreakpoints.push_back(bp.GetJson());
	}

	// update watchpoints
	empty &= m_watchpoints.GetAll().empty();
	debugDataJ["watchpoints"] = {};
	auto& debugWatchpoints = debugDataJ["watchpoints"];
	for (const auto& [id, wp] : m_watchpoints.GetAll())
	{
		debugWatchpoints.push_back(wp.GetJson());
	}
	
	// save if the debug data is not empty or the file exists
	if (!empty || dev::IsFileExist(m_debugPath)) dev::SaveJson(m_debugPath, debugDataJ);
}