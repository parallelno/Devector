#pragma once
#ifndef DEV_STRING_UTILS_H
#define DEV_STRING_UTILS_H

#include <string>
#include <vector>

namespace dev
{
	auto fast_atof(const char* _s) 
		->double;
	auto fast_atoi(const char* _s)
		->int;

	auto Split(const std::string& _s, char _delim)
		->std::vector<std::string>;

	const std::wstring StrToStrW(const std::string& _s);
	const std::string StrWToStr(const std::wstring& _s);
}
#endif // !DEV_STRING_UTILS_H
