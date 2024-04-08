#include "Breakpoint.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>
#include "Utils/StringUtils.h"
#include "Utils/Utils.h"

dev::Breakpoint::Breakpoint(const GlobalAddr _globalAddr, 
	const Status _status, const bool _autoDel,
	const std::string& _comment)
	: 
	m_globalAddr(_globalAddr), m_status(_status), 
	m_autoDel(_autoDel), m_comment(_comment)
{}

auto dev::Breakpoint::GetStatus() const -> Status { return m_status; }

auto dev::Breakpoint::GetStatusI() const -> int { return static_cast<int>(m_status); }

void dev::Breakpoint::SetStatus(const Status _status) {	m_status = _status; }

auto dev::Breakpoint::GetComment() const ->const std::string& {	return m_comment; }

auto dev::Breakpoint::GetConditionS() const
->std::string
{
	return "";
}

bool dev::Breakpoint::IsActive() const { return m_status == Status::ACTIVE; }

auto dev::Breakpoint::IsActiveS() const -> const char* { return m_status == Status::ACTIVE ? "X" : "-"; }

bool dev::Breakpoint::IsAutoDel() const { return m_autoDel; }

auto dev::Breakpoint::CheckStatus() const -> bool {	return m_status == Status::ACTIVE; }

auto dev::Breakpoint::GetGlobalAddr() const -> GlobalAddr { return m_globalAddr; }

void dev::Breakpoint::Print() const
{
	std::printf("0x%06x, active: %d \n", m_globalAddr, GetStatusI());
}