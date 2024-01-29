// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#pragma once
#ifndef BOT_UTILS_H
#define BOT_UTILS_H

#include <chrono>
#include <format>
#include <iostream>
#include <mutex>

#include "utils/globals.h"
#include <string>
#include <vector>
#include <functional>
#include <filesystem>

namespace dev 
{

// MACRO
template<typename T, typename = void>
constexpr bool is_defined = false;

template<typename T>
constexpr bool is_defined<T, decltype(typeid(T), void())> = true;

// "var"
#define VAR_TO_STR(var) (#var)
// to convert a value of __LINE__ to string
#define TO_STRING2(x) #x
#define TO_STRING(x) TO_STRING2(x)

#define ENUM_TO_STR(res, EmunType, EnumItem) case EmunType::##EnumItem : \
		res = #EnumItem; \
		break;


// VA_COUNT
#define ELEVENTH_ARGUMENT(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, ...) a11
#define VA_COUNT(...) ELEVENTH_ARGUMENT(dummy, ## __VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

// enum to a const char* array
#define ENUM_TO_CHAR_ARRAY(arrayName, ...) const char* arrayName[VA_COUNT(__VA_ARGS__)] = { __VA_ARGS__ }
	//--------------------------------------------------------------
	//
	// UTILS
	//
	//--------------------------------------------------------------

	static std::mutex logMutex;
	// Local time
	template <typename... Args>
	constexpr void Log(const std::string& _fmt, Args&&... args)
	{
		logMutex.lock();

		using namespace std::chrono;

		std::cout << "Local time " <<
			floor<seconds>(current_zone()->to_local(system_clock::now())) <<
			"  " <<
			std::vformat(_fmt, std::make_format_args(args...)) <<
			std::endl;

		logMutex.unlock();
	}
	
	// NY time
	template <typename... Args>
	constexpr void LogNY(const bool announce, 
		const std::string& _fmt, Args&&... args)
	{
		logMutex.lock();
		
		if (announce) std::cout << "__________________________________________________________________________________________\n";

		using namespace std::chrono;
		std::cout << "NY time " <<
			floor<seconds>(zoned_time("America/New_York", system_clock::now()).get_local_time()) <<
			"  " <<
			std::vformat(_fmt, std::make_format_args(args...)) <<
			std::endl;

		if (announce) std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n";
		logMutex.unlock();
	}

	void RunApp(const std::string& dir, const std::string& appName);
	void ThreadSleep(double seconds);

	template <typename T>
	int BinarySearch(const T* arr, int64_t l, int64_t r, T x)
	{
		if (r >= l) {
			int mid = l + (r - l) / 2;
			if (arr[mid] == x)
				return mid;
			if (arr[mid] > x)
				return BinarySearch(arr, l, mid - 1, x);
			return BinarySearch(arr, mid + 1, r, x);
		}
		return -1;
	}

	using SendEmergencyMsgT = std::function<void(const std::string&)>;
	static void Exit(
		const std::string& _errStr,
		dev::ErrCode _err,
		SendEmergencyMsgT _sendEmergencyMsg = nullptr,
		const bool _timezoneNY = false)
	{
		if (_timezoneNY)
		{
			dev::LogNY(true, "EXIT. err code: {}, msg: {}",
				_err, _errStr);
		}
		else
		{
			dev::Log("EXIT. err code: {}, msg: {}",
				_err, _errStr);
		}

		if (_sendEmergencyMsg) _sendEmergencyMsg(_errStr);
		
		dev::ThreadSleep(30.0);
		exit(_err);
	}
	//--------------------------------------------------------------
	//
	// MATH
	//
	//--------------------------------------------------------------

	inline double lerp(double _a, double _b, double _c)
	{
		return _a * _c + _b * (1.0 - _c);
	}
	inline int sign(int _val) 
	{
		return (0 < _val) - (_val < 0);
	}
	inline double RoundToCents(double _price)
	{
		return round(_price * 100.0) / 100.0;
	}
	inline double rnd(const double _min, const double _max)
	{
		return (double)rand() / RAND_MAX * (_max - _min) + _min;
	}
	inline double rnd(const double _max) 
	{
		return rnd(0.0, _max);
	}

	//--------------------------------------------------------------
	//
	// FILES
	//
	//--------------------------------------------------------------
	inline bool IsFileExist(const std::string& _path)
	{
		if (std::filesystem::exists(_path)) {
			return true;
		}
		return false;
	}
	auto LoadTextFile(const std::string& path) 
		->std::vector<std::string>;
	void DeleteFiles(const std::string& folder, const std::string& mask = "*");
	auto GetAllFilesPaths(const std::string& _dir, const std::string& _prefix = "")
		->const std::vector<std::string>;
	auto GetStockNameFromTickDataPath(const std::string& _path)
		->const std::string;
	inline auto GetTickDataFileName(const std::string& _dateStr)
		->const std::string
	{
		return _dateStr + DB_FILENAME_POSTFIX;
	}
	inline auto GetTickDataPath(const std::string& _dir, const std::string& _dateStr)
		->const std::string
	{
		return _dir + "\\" + _dateStr + DB_FILENAME_POSTFIX;
	}
	inline auto GetTradingReportFileName(const std::string& stockName)
		->const std::string
	{
		return stockName + DB_FILENAME_POSTFIX;
	}
	inline auto GetTradingReportPath(const std::string& _stockName, const std::string& _dir)
		->const std::string
	{
		return _dir + GetTradingReportFileName(_stockName);
	}

} // namespace dev
#endif //!BOT_UTILS_H