#pragma once

#include <chrono>
#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include <filesystem>

#include "utils/consts.h"
#include "utils/result.h"

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


// PERFORMANCE
#define PERT_TEST_START(_loops) \
	{ \
		volatile int __dummy = 0; \
		auto __pert_test_start = std::chrono::high_resolution_clock::now(); \
		for( int __perfIter = 0; __perfIter < _loops; __perfIter++ ) \
		{ \
			__dummy++; // prevent the compiler from optimizing out the loop

#define PERT_TEST_END(_text) \
		} \
		auto __pert_test_end = std::chrono::high_resolution_clock::now(); \
		auto __pert_test_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(__pert_test_end - __pert_test_start).count(); \
		dev::Log("PERF TEST: {} takes sec: {}", _text, static_cast<double>( __pert_test_duration) / 1e9 ); \
	}

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

	void RunApp(const std::string& dir, const std::string& appName);
	void OsOpenInShell(const char* path);
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
		SendEmergencyMsgT _sendEmergencyMsg = static_cast<SendEmergencyMsgT>(nullptr))
	{
		dev::Log("EXIT. err code: {}, msg: {}",
				(int)_err, _errStr);

		if (_sendEmergencyMsg) _sendEmergencyMsg(_errStr);
		
		dev::ThreadSleep(30.0);
		exit((int)_err);
	}

	void CopyToClipboard(const std::string& _str);

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
	inline double rnd(const double _min, const double _max)
	{
		return (double)rand() / RAND_MAX * (_max - _min) + _min;
	}
	inline double rnd(const double _max) 
	{
		return rnd(0.0, _max);
	}

	template <typename T, typename M>
	inline T Max(T a, M b)
	{
		return (a > b) ? a : b;
	}

	template <typename T, typename M>
	inline T Min(T a, M b)
	{
		return (a < b) ? a : b;
	}

	template <typename T>
	inline T Abs(T a)
	{
		return (a > 0) ? a : -a;
	}

	//--------------------------------------------------------------
	//
	// FILES
	//
	//--------------------------------------------------------------
	inline bool IsFileExist(const std::string& _path)
	{
		return std::filesystem::exists(_path);
	}

	auto LoadTextFile(const std::string& _path) ->std::string;

	auto LoadFile(const std::string& _path)
		-> dev::Result<std::vector<uint8_t>>;

	bool SaveFile(const std::string& _path, 
		const std::vector<uint8_t>& _data, const bool _override = true);

	void DeleteFiles(const std::string& _dir, const std::string& _mask = "*");

	size_t GetFileSize(const std::string& _path);
	
	auto GetDir(const std::string& _path)
		-> std::string;

	auto GetFilename(const std::string& _path)
		-> std::string;

	auto GetExt(const std::string& _path)
		-> std::string;

	auto GetDirStemExt(const std::string& _path)
		-> std::tuple<std::string, std::string, std::string>;

	auto GetExecutableDir() -> std::string;
}