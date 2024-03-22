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
	using ErrCode = int;

	constexpr static ErrCode NO_ERRORS = 0;
	constexpr static ErrCode ERROR_UNSPECIFIED = 1;
	constexpr static ErrCode ERROR_NO_FILES = 2;
}

#endif // !DEV_GLOBALS_H