#include "Breakpoint.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>
#include "Utils/StringUtils.h"
#include "Utils/Utils.h"

dev::Breakpoint::Breakpoint(const uint32_t _globalAddr, const Status _status)
	: m_globalAddr(_globalAddr), m_status(_status)
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

auto dev::Breakpoint::IsActiveS() const -> const char*
{
	return m_status == Status::ENABLED ? "X" : "-";
}

auto dev::Breakpoint::CheckStatus() const
->const bool
{
	return m_status == Status::ENABLED;
}

void dev::Breakpoint::Print() const
{
	std::printf("0x%06x, active: %d \n", m_globalAddr, GetStatusI());
}