// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "utils.h"
#include "utils/globals.h"
#include "utils/stringUtils.h"
#include "utils/result.h"

#include <cstdarg>
#include <fstream>

void dev::RunApp(const std::wstring& _dir, const std::wstring& _appName) 
{
	if ( !IsFileExist(_dir + _appName) )
	{
		return;
	}
	const std::wstring command = L"cd " + _dir + L" && " + _appName;
	system(dev::StrWToStr(command).c_str());
}

void dev::ThreadSleep(double _seconds) 
{
	std::this_thread::sleep_for(std::chrono::milliseconds((long long)(_seconds * 1000.0)));
}

auto dev::LoadTextFile(const std::wstring& path)
-> std::vector<std::string>
{	
	if (!IsFileExist(path)) {
		return {};
	}
	std::ifstream ifs(path);
	std::string fileStr = std::string(std::istreambuf_iterator<char>(ifs),
		std::istreambuf_iterator<char>());

	return dev::Split(fileStr, '\n');
}

auto dev::LoadFile(const std::wstring& path)
-> dev::Result<std::vector<uint8_t>>
{
	if (!IsFileExist(path)) {
		return {};
	}

	// Open the file in binary mode
	std::ifstream file(path, std::ios::binary);

	// Check if the file is opened successfully
	if (!file.is_open()) {
		std::wcerr << L"Failed to open file: " << path << std::endl;
		return false;
	}

	std::vector<uint8_t> data(std::istreambuf_iterator<char>(file), {});

	return { std::move(data) };
}

void dev::DeleteFiles(const std::wstring& _dir, const std::wstring& _mask)
{
	const std::wstring command = L"del /Q " + _dir + _mask;
	system(dev::StrWToStr(command).c_str());
}

size_t dev::GetFileSize(const std::wstring& filename)
{
	std::filesystem::path p{ filename };
	return std::filesystem::file_size(p);
}
