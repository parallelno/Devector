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
	const Status _status, const bool _autoDel, const std::string& _comment)
	:
	m_data(_addr, _mappingPages, _status, _autoDel), m_comment(_comment)
{}

void dev::Breakpoint::Update(const Addr _addr,
	const uint8_t _mappingPages,
	const Status _status, const bool _autoDel, const std::string& _comment)
{
	m_data = Data(_addr, _mappingPages, _status, _autoDel), m_comment = _comment;
}
/*
auto dev::Breakpoint::GetConditionS() const
->std::string
{
	return "";
}
*/
auto dev::Breakpoint::IsActiveS() const -> const char* { return m_data.status == Status::ACTIVE ? "X" : "-"; }

bool dev::Breakpoint::CheckStatus(const uint8_t _mappingModeRam, const uint8_t _mappingPageRam) const
{	
	auto mapping = _mappingModeRam == 0 ? 1 : 1 << (_mappingPageRam + 1);
	return m_data.status == Status::ACTIVE && mapping & m_data.mappingPages;
}

void dev::Breakpoint::Print() const
{
	std::printf("0x%04x, active: %d, mappingPages: %d, autoDel: %d \n", m_data.addr, m_data.status, m_data.mappingPages, m_data.autoDel);
}

auto dev::Breakpoint::Data::GetAddrMappingS() const
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