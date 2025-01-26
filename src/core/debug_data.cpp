#include <format>

#include "debug_data.h"

dev::DebugData::DebugData(Hardware& _hardware)
	: m_hardware(_hardware)
{}

/*
void dev::DebugData::Reset()
{
	m_debugPath.clear();
	m_labels.clear();
	m_consts.clear();
	m_comments.clear();
	
	m_breakpoints.Clear();
	m_watchpoints.Clear();
}
*/

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

auto dev::DebugData::GetLabels(const Addr _addr) const -> const LabelList*
{
	auto labelsI = m_labels.find(_addr);
	return labelsI != m_labels.end() ? &labelsI->second : nullptr;
}
void dev::DebugData::GetFilteredLabels(FilteredElements& _out, const std::string& _filter) const
{
	_out.clear();
	for (const auto& [globalAddr, labels] : m_labels) 
	{
		for(const auto& label : labels)
		{
			if (label.find(_filter) == std::string::npos) continue;

			_out.push_back({ label, globalAddr, std::format("0x{:06x}", globalAddr) });
		}
	}

	// alphabetical	sort
	std::sort(_out.begin(), _out.end(), [](const auto& lhs, const auto& rhs) {
		return std::get<0>(lhs) < std::get<0>(rhs);
	});
}

void dev::DebugData::SetLabels(const Addr _addr, const LabelList& _labels)
{
	if (_labels.empty()) {
		auto labelsI = m_labels.find(_addr);
		if (labelsI != m_labels.end()) m_labels.erase(labelsI);
	}
	else {
		m_labels[_addr] = _labels;
	}

	m_labelsUpdates++;
}

void dev::DebugData::AddLabel(const Addr _addr, const std::string& _label)
{
	auto labelsI = m_labels.find(_addr);
	if (labelsI == m_labels.end()) {
		m_labels[_addr] = LabelList{ _label };
	}
	else {
		labelsI->second.push_back(_label);
	}
	m_labelsUpdates++;
}

void dev::DebugData::DelLabel(const Addr _addr, const std::string& _label)
{
	auto labelsI = m_labels.find(_addr);
	if (labelsI != m_labels.end()) 
	{
		auto labelI = std::find(labelsI->second.begin(), labelsI->second.end(), _label);
		if (labelI != labelsI->second.end()) 
		{
			labelsI->second.erase(labelI);
			if (labelsI->second.empty()) {
				m_labels.erase(labelsI);
			}
			m_labelsUpdates++;
		}
	}
}

void dev::DebugData::DelLabels(const Addr _addr)
{
	auto labelsI = m_labels.find(_addr);
	if (labelsI == m_labels.end()) return;

	m_labels.erase(labelsI);
	m_labelsUpdates++;
}

void dev::DebugData::DelAllLabels()
{
	m_labels.clear();
	m_labelsUpdates++;
}

void dev::DebugData::RenameLabel(const Addr _addr, const std::string& _oldLabel, const std::string& _newLabel)
{
	auto labelsI = m_labels.find(_addr);
	if (labelsI != m_labels.end()) 
	{
		auto labelI = std::find(labelsI->second.begin(), labelsI->second.end(), _oldLabel);
		if (labelI != labelsI->second.end()) 
		{
			*labelI = _newLabel;
			m_labelsUpdates++;
		}
	}
}

auto dev::DebugData::GetConsts(const Addr _addr) const -> const LabelList*
{
	auto constsI = m_consts.find(_addr);
	return constsI != m_consts.end() ? &constsI->second : nullptr;
}

void dev::DebugData::GetFilteredConsts(FilteredElements& _out, const std::string& _filter) const
{
	_out.clear();
	for (const auto& [globalAddr, consts] : m_consts) 
	{
		for(const auto& const_ : consts)
		{
			if (const_.find(_filter) == std::string::npos) continue;

			_out.push_back({ const_, globalAddr, std::format("0x{:06x}", globalAddr) });
		}
	}

	// alphabetical	sort
	std::sort(_out.begin(), _out.end(), [](const auto& lhs, const auto& rhs) {
		return std::get<0>(lhs) < std::get<0>(rhs);
	});
}

void dev::DebugData::SetConsts(const Addr _addr, const LabelList& _consts)
{
	if (_consts.empty()) {
		// del the consts at the addr if the list of consts is empty
		auto constsI = m_consts.find(_addr);
		if (constsI != m_consts.end()) m_consts.erase(constsI);
	}
	else {
		m_consts[_addr] = _consts;
	}

	m_constsUpdates++;
}

void dev::DebugData::AddConst(const Addr _addr, const std::string& _const)
{
	auto constsI = m_consts.find(_addr);
	if (constsI == m_consts.end()) {
		m_consts[_addr] = LabelList{ _const };
	}
	else {
		constsI->second.push_back(_const);
	}
	m_constsUpdates++;
}

void dev::DebugData::DelConst(const Addr _addr, const std::string& _const)
{
	auto constsI = m_consts.find(_addr);
	if (constsI != m_consts.end()) {
		auto constI = std::find(constsI->second.begin(), constsI->second.end(), _const);
		if (constI != constsI->second.end()) 
		{
			constsI->second.erase(constI);
			if (constsI->second.empty()) {
				m_consts.erase(constsI);
			}
			m_constsUpdates++;
		}
	}
}

void dev::DebugData::DelConsts(const Addr _addr)
{
	auto constsI = m_consts.find(_addr);
	if (constsI == m_consts.end()) return;

	m_consts.erase(constsI);
	m_constsUpdates++;
}

void dev::DebugData::DelAllConsts()
{
	m_consts.clear();
	m_constsUpdates++;
}

void dev::DebugData::RenameConst(const Addr _addr, const std::string& _oldConst, const std::string& _newConst)
{
	auto constsI = m_consts.find(_addr);
	if (constsI != m_consts.end()) {
		auto constI = std::find(constsI->second.begin(), constsI->second.end(), _oldConst);
		if (constI != constsI->second.end()) {
			*constI = _newConst;
			m_constsUpdates++;
		}
	}
}


auto dev::DebugData::GetComment(const Addr _addr) const
-> const std::string*
{
	auto commentI = m_comments.find(_addr);
	return commentI != m_comments.end() ? &commentI->second : nullptr;
}

void dev::DebugData::GetFilteredComments(FilteredElements& _out, const std::string& _filter) const
{	
	_out.clear();
	for (const auto& [globalAddr, comment] : m_comments) 
	{
		if (comment.find(_filter) == std::string::npos) continue;

		_out.push_back({ comment, globalAddr, std::format("0x{:06x}", globalAddr) });
	}

	// sort by addr	
	std::sort(_out.begin(), _out.end(), [](const auto& lhs, const auto& rhs) {
		return std::get<1>(lhs) < std::get<1>(rhs);
	});
}

void dev::DebugData::SetComment(const Addr _addr, const std::string& _comment)
{
	m_comments[_addr] = _comment;
	m_commentsUpdates++;
}

void dev::DebugData::DelComment(const Addr _addr)
{
	auto commentI = m_comments.find(_addr);
	if (commentI == m_comments.end()) return;
	m_comments.erase(commentI);
	m_commentsUpdates++;
}

void dev::DebugData::DelAllComments()
{
	m_comments.clear();
	m_commentsUpdates++;
}

auto dev::DebugData::GetMemoryEdit(const Addr _addr) const 
-> const MemoryEdit*
{
	auto editsI = m_memoryEdits.find(_addr);
	return editsI != m_memoryEdits.end() ? &editsI->second : nullptr;
}

void dev::DebugData::SetMemoryEdit(const MemoryEdit& _edit)
{
	m_memoryEdits[_edit.globalAddr] = _edit;
	m_editsUpdates++;
}

void dev::DebugData::DelMemoryEdit(const Addr _addr)
{
	auto editI = m_memoryEdits.find(_addr);
	if (editI == m_memoryEdits.end()) return;
	m_memoryEdits.erase(editI);
	m_editsUpdates++;
}

void dev::DebugData::DelAllMemoryEdits()
{
	m_memoryEdits.clear();
	m_editsUpdates++;
}

void dev::DebugData::GetFilteredMemoryEdits(FilteredElements& _out, const std::string& _filter) const
{
	_out.clear();
	for (const auto& [globalAddr, edit] : m_memoryEdits)
	{
		if (edit.comment.find(_filter) == std::string::npos) continue;
		_out.push_back({ edit.comment, globalAddr, edit.AddrToStr() });
	}
}

void dev::DebugData::LoadDebugData(const std::string& _path)
{
	// check if the file exists
	auto debugDir = dev::GetDir(_path);
	m_debugPath = debugDir + "/" + dev::GetFilename(_path) + ".json";

	
	m_labelsUpdates++;
	m_labels.clear();
	m_constsUpdates++;
	m_consts.clear();
	m_commentsUpdates++;
	m_comments.clear();	
	m_editsUpdates++;	
	m_memoryEdits.clear();
	
	m_breakpoints.Clear();
	m_watchpoints.Clear();	

	// init empty dictionaries when there is no file found
	if (!dev::IsFileExist(m_debugPath)) return;
	
	auto debugDataJ = nlohmann::json::object();
	try
	{
		debugDataJ = LoadJson(m_debugPath);	
	}
	catch (const std::exception& e)
	{
		dev::Log("The debug data file is corrupted: {}", e.what());
		return;
	}

	// add labels
	if (debugDataJ.contains("labels")) {
		for (auto& [str, addrS] : debugDataJ["labels"].items())
		{
			Addr addr = dev::StrHexToInt(addrS.get<std::string>().c_str());
			m_labels.emplace(addr, LabelList{}).first->second.emplace_back(str);
		}
	}
	
	// add consts
	if (debugDataJ.contains("consts")) {
		for (auto& [str, addrS] : debugDataJ["consts"].items())
		{
			Addr addr = dev::StrHexToInt(addrS.get<std::string>().c_str());
			m_consts.emplace(addr, LabelList{}).first->second.emplace_back(str);
		}
	}

	// add comments
	if (debugDataJ.contains("comments")) {
		for (auto& [addrS, str] : debugDataJ["comments"].items())
		{
			Addr addr = dev::StrHexToInt(addrS.c_str());
			m_comments.emplace(addr, str);
		}
	}

	// add memory edits
	if (debugDataJ.contains("memoryEdits")) {
		for (auto& editJ : debugDataJ["memoryEdits"])
		{
			MemoryEdit edit{ editJ };
			m_memoryEdits.emplace(edit.globalAddr, edit);
			// inject memory edits
			if (edit.active) m_hardware.Request(Hardware::Req::SET_BYTE_GLOBAL, { {"addr", edit.globalAddr}, {"data", edit.value} });
		}
	}

	// add breakpoints
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

	// update memory edits
	empty &= m_memoryEdits.empty();
	debugDataJ["memoryEdits"] = {};
	auto& debugMemoryEdits = debugDataJ["memoryEdits"];
	for (const auto& [addr, memoryEdit] : m_memoryEdits)
	{
		debugMemoryEdits.push_back(memoryEdit.ToJson());
	}

	// update breakpoints
	empty &= m_breakpoints.GetAll().empty();
	debugDataJ["breakpoints"] = {};
	auto& debugBreakpoints = debugDataJ["breakpoints"];
	for (const auto& [addr, bp] : m_breakpoints.GetAll())
	{
		debugBreakpoints.push_back(bp.ToJson());
	}

	// update watchpoints
	empty &= m_watchpoints.GetAll().empty();
	debugDataJ["watchpoints"] = {};
	auto& debugWatchpoints = debugDataJ["watchpoints"];
	for (const auto& [id, wp] : m_watchpoints.GetAll())
	{
		debugWatchpoints.push_back(wp.ToJson());
	}
	
	// save if the debug data is not empty or the file exists
	if (!empty || dev::IsFileExist(m_debugPath)) dev::SaveJson(m_debugPath, debugDataJ);
}