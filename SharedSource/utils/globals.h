// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#pragma once
#ifndef DEV_GLOBALS_H
#define DEV_GLOBALS_H

#include <map>
#include <tuple>
#include <vector>
#include <string>
#include <memory>

namespace dev
{
#define DB_FILENAME_POSTFIX ".db"
	using ErrCode = int;

	constexpr static ErrCode NO_ERRORS = 0;
	constexpr static ErrCode ERROR_UNSPECIFIED = 1;
	constexpr static ErrCode ERROR_NO_FILES = 2;

	using Paths = std::map<std::string, std::string>;
	using Sizes = std::vector<size_t>;

} // namespace dev

#endif // !DEV_GLOBALS_H