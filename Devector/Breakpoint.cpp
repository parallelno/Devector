#include "Breakpoint.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>
#include "Utils/StringUtils.h"
#include "Utils/Utils.h"

dev::Breakpoint::Breakpoint(const GlobalAddr _globalAddr, const Status _status, const std::string& _comment)
	: 
	m_globalAddr(_globalAddr), m_status(_status), m_comment(_comment)
{}

auto dev::Breakpoint::GetStatus() const
->const Status
{
	return m_status;
}

auto dev::Breakpoint::GetStatusI() const
->const int
{
	return static_cast<int>(m_status);
}

auto dev::Breakpoint::GetComment() const
->const std::string&
{
	return m_comment;
}

auto dev::Breakpoint::IsActiveS() const -> const char*
{
	return m_status == Status::ACTIVE ? "X" : "-";
}

auto dev::Breakpoint::CheckStatus() const
->const bool
{
	return m_status == Status::ACTIVE;
}

void dev::Breakpoint::Print() const
{
	std::printf("0x%06x, active: %d \n", m_globalAddr, GetStatusI());
}