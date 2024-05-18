#include <sstream>

#include "Utils/StrUtils.h"
#include <locale>
#include <codecvt>

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
	const std::string s(_ws.begin(), _ws.end());
	return s;
}
/*
auto dev::Utf8ToStrW(const std::string& _s)
-> std::wstring
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(_s);
}
*/
const char* dev::BoolToStrC(const bool _val, bool _text)
{
	if (_text)
	{
		static const char* false_s = "False";
		static const char* true_s = "True";
		return _val ? true_s : false_s;
	}

	static const char* false_s0 = "0";
	static const char* true_s0 = "1";
	return _val ? true_s0 : false_s0;
}

auto dev::BoolToStr(const bool _val, bool _text)
-> const std::string
{
	if (_text)
	{
		static const std::string false_s = "False";
		static const std::string true_s = "True";
		return _val ? true_s : false_s;
	}

	static const std::string false_s0 = "0";
	static const std::string true_s0 = "1";
	return _val ? true_s0 : false_s0;
}

auto dev::StrHexToInt(const char* _str)
-> int
{
	char* end;
	return strtol(_str, &end, 16);
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

auto dev::StrToUpper(std::wstring _str)
-> std::wstring
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