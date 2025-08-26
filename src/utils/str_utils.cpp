#include <sstream>
#include <locale>
#include <codecvt>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <string>

#include "utils/str_utils.h"

double dev::fast_atof(const char* _s)
{
	double rez = 0.0, fact = 1.0;
	if (*_s == '-') {
		_s++;
		fact = -1.0;
	};
	for (int point_seen = 0; *_s; _s++) {
		if (*_s == '.') {
			point_seen = 1;
			continue;
		};
		char d = *_s - '0';
		if (point_seen) fact /= 10.0;
		rez = rez * 10.0 + (double)d;
	};
	return rez * fact;
}

int dev::fast_atoi(const char* _s)
{
	int val = 0;
	while (*_s) {
		val = val * 10 + (*_s++ - '0');
	}
	return val;
}

auto dev::Split(const std::string& _s, char _delim)
->std::vector<std::string>
{
	std::vector<std::string> result;
	std::stringstream ss(_s);
	std::string item;

	while (std::getline(ss, item, _delim)) {
		result.push_back(item);
	}

	return result;
}

auto dev::StrToStrW(const std::string& _s)
-> const std::wstring
{
	std::wstringstream cls;
	cls << _s.c_str();
	return cls.str();
}

auto dev::StrWToStr(const std::wstring& _ws)
-> const std::string
{
	//const std::string s(_ws.begin(), _ws.end());
	//return s;
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(_ws);
}
/*
auto dev::Utf8ToStrW(const std::string& _s)
-> std::wstring
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(_s);
}
*/
const char* dev::BoolToStrC(const bool _val, int _mode)
{
	static const char* false_s0 = "False";
	static const char* true_s0 = "True";
	static const char* false_s1 = "0";
	static const char* true_s1 = "1";
	static const char* false_s2 = "Off";
	static const char* true_s2 = "On";
	switch (_mode) {
	case 0:
		return _val ? true_s0 : false_s0;
	case 1:
		return _val ? true_s1 : false_s1;
	case 2:
		return _val ? true_s2 : false_s2;
	}
	return "NOT_SUPPORTED";
}

auto dev::GetSubstringCount(const std::string& _str, const std::string& _substr)
-> int
{
	int count = 0;
	size_t pos = 0;

	while ((pos = _str.find(_substr, pos)) != std::string::npos) {
		++count;
		pos += _str.length();
	}

	return count;
}

auto dev::StrToUpperW(std::wstring _str)
-> std::wstring
{
	for (auto& ch : _str) {
		ch = std::toupper(ch);
	}
	return _str;
}

auto dev::StrToUpper(std::string _str)
-> std::string
{
	for (auto& ch : _str) {
		ch = std::toupper(ch);
	}
	return _str;
}

// trim from end (copy)
auto dev::TrimSpacesRight(std::wstring _str)
-> std::wstring
{
	_str.erase(
		std::find_if(
			_str.rbegin(), _str.rend(), [](int ch) { return !std::isspace(ch);}
		).base(), _str.end());

	return _str;
}

/// <summary>
/// init the C strings to convert uint16_t to str with a prefix 0x
/// </summary>
#define I16_ADDRS_LEN 7	// sizeof("0xFFFF");
static char addrsS[0x10000 * I16_ADDRS_LEN]; // for fast Addr to AddrS conversion

#define I8_ADDRS_LEN 5	// sizeof("0xFF");
static char smallAddrsS[0x100 * I8_ADDRS_LEN]; // for fast Addr to AddrS conversion

void InitAddrsS()
{
	static bool inited = false;
	if (inited) return;
	inited = true;

	char addrS[5]; // "FFFF"
	for (int i = 0, addr = 0; i < sizeof(addrsS); addr++)
	{
#if defined(_WIN32)
		sprintf_s(addrS, 5, "%04X", addr);
#else
		sprintf(addrS, "%04X", addr);
#endif
		addrsS[i++] = '0';
		addrsS[i++] = 'x';
		addrsS[i++] = addrS[0];
		addrsS[i++] = addrS[1];
		addrsS[i++] = addrS[2];
		addrsS[i++] = addrS[3];
		addrsS[i++] = 0;
	}

	char smallAddrS[3]; // "FF"
	for (int i = 0, addr = 0; i < sizeof(smallAddrsS); addr++)
	{
#if defined(_WIN32)
		sprintf_s(smallAddrS, 3, "%02X", addr);
#else
		sprintf(smallAddrS, "%02X", addr);
#endif
		smallAddrsS[i++] = '0';
		smallAddrsS[i++] = 'x';
		smallAddrsS[i++] = smallAddrS[0];
		smallAddrsS[i++] = smallAddrS[1];
		smallAddrsS[i++] = 0;
	}
}

struct IniterAddrsS {
	IniterAddrsS() { InitAddrsS(); }
};
static IniterAddrsS initerAddrsS;


auto dev::Uint16ToStrC0x(const uint16_t _addr)
-> const char*
{
	return addrsS + _addr * I16_ADDRS_LEN;
}

auto dev::Uint8ToStrC0x(const uint8_t _addr)
-> const char*
{
	return smallAddrsS + _addr * I8_ADDRS_LEN;
}


auto dev::Uint16ToStrC(const uint16_t _addr)
-> const char*
{
	return addrsS + _addr * I16_ADDRS_LEN + 2;
}


auto dev::Uint8ToStrC(const uint8_t _addr)
-> const char*
{
	return smallAddrsS + _addr * I8_ADDRS_LEN + 2;
}