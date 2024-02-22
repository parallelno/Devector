#include <sstream>

#include "Utils/StringUtils.h"

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

auto dev::StrToStrW(const std::string& s)
-> const std::wstring
{
	std::wstringstream cls;
	cls << s.c_str();
	return cls.str();
}

auto dev::StrWToStr(const std::wstring& ws)
-> const std::string
{
	const std::string s(ws.begin(), ws.end());
	return s;
}