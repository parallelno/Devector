#include <cstdarg>
#include <fstream>

#include "Utils/Utils.h"
#include "Utils/Globals.h"
#include "Utils/StringUtils.h"
#include "Utils/Result.h"

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

auto dev::LoadTextFile(const std::wstring& _path)
-> std::vector<std::string>
{	
	if (!IsFileExist(_path)) {
		return {};
	}
	std::ifstream ifs(_path);
	std::string fileStr = std::string(std::istreambuf_iterator<char>(ifs),
		std::istreambuf_iterator<char>());

	return dev::Split(fileStr, '\n');
}

auto dev::LoadFile(const std::wstring& _path)
-> Result<std::vector<uint8_t>>
{
	if (!IsFileExist(_path)) {
		return {};
	}

	// Open the file in binary mode
	std::ifstream file(_path, std::ios::binary);

	// Check if the file is opened successfully
	if (!file.is_open()) {
		std::wcerr << L"Failed to open file: " << _path << std::endl;
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

size_t dev::GetFileSize(const std::wstring& _path)
{
	std::filesystem::path p{ _path };
	return std::filesystem::file_size(p);
}

auto dev::GetDir(const std::wstring& _path)
->std::wstring
{
	std::filesystem::path p{ _path };
	return p.parent_path().wstring();
}
