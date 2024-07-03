#pragma once

#include <string>
#include <vector>

namespace dev
{
	auto fast_atof(const char* _s) 
		-> double;
	auto fast_atoi(const char* _s)
		-> int;

	auto Split(const std::string& _s, char _delim)
		-> std::vector<std::string>;

	auto StrToStrW(const std::string& _s) -> const std::wstring;
	auto Utf8ToStrW(const std::string& _s) -> std::wstring;
	auto StrWToStr(const std::wstring& _s) -> const std::string;
	auto BoolToStrC(const bool _val, int _mode = 0) -> const char*;
	auto StrHexToInt(const char* _str) -> int;
	auto GetSubstringCount(const std::string& _str, const std::string& _substr) -> int;
	auto StrToUpper(const std::wstring _str) -> std::wstring;
	inline auto TrimSpacesRight(std::wstring _str) -> std::wstring;
	auto Uint16ToStrC0x(const uint16_t _addr) -> const char*;
	auto Uint8ToStrC0x(const uint8_t _addr) -> const char*;
	auto Uint16ToStrC(const uint16_t _addr) -> const char*;
	auto Uint8ToStrC(const uint8_t _addr) -> const char*;
}
