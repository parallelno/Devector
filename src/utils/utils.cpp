#include <cstdarg>
#include <fstream>
#include <iostream>
#include <thread>

#include "utils/utils.h"
#include "utils/consts.h"
#include "utils/str_utils.h"
#include "utils/result.h"

void dev::RunApp(const std::wstring& _dir, const std::wstring& _appName) 
{
	if ( !IsFileExist(_dir + _appName) )
	{
		return;
	}
	const std::wstring command = L"cd " + _dir + L" && " + _appName;
	system(dev::StrWToStr(command).c_str());
}

void dev::OsOpenInShell(const char* path)
{
#ifdef _WIN32
	// Note: executable path must use backslashes!
	::ShellExecuteA(NULL, "open", path, NULL, NULL, SW_SHOWDEFAULT);
#else
#if __APPLE__
	const char* open_executable = "open";
#else
	const char* open_executable = "xdg-open";
#endif
	char command[256];
	snprintf(command, 256, "%s \"%s\"", open_executable, path);
	system(command);
#endif
}

void dev::ThreadSleep(double _seconds) 
{
	std::this_thread::sleep_for(std::chrono::milliseconds((long long)(_seconds * 1000.0)));
}

void dev::CopyToClipboard(const std::string& _str) {
#if defined(_WIN32)
		if (OpenClipboard(nullptr)) {
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, _str.size() + 1);
			if (hMem) {
				memcpy(GlobalLock(hMem), _str.c_str(), _str.size() + 1);
				GlobalUnlock(hMem);
				EmptyClipboard();
				SetClipboardData(CF_TEXT, hMem);
				GlobalFree(hMem);
			}
			CloseClipboard();
		}
#elif defined(__linux__)
		std::ofstream("/dev/clipboard") << _str;
#endif
	}

auto dev::LoadTextFile(const std::wstring& _path)
-> std::string
{	
	if (!IsFileExist(_path)) {
		return {};
	}

#if defined(_WIN32)
	std::ifstream ifs(_path);
#elif defined(__linux__)
	std::ifstream ifs(dev::StrWToStr(_path));
#endif	

	std::string data = std::string(std::istreambuf_iterator<char>(ifs),
		std::istreambuf_iterator<char>());

	return data;
}

auto dev::LoadFile(const std::wstring& _path)
-> Result<std::vector<uint8_t>>
{
	if (!IsFileExist(_path)) {
		return {};
	}

	// Open the file in binary mode
#if defined(_WIN32)
	std::ifstream file(_path, std::ios::binary);
#elif defined(__linux__)
	std::ifstream file(dev::StrWToStr(_path), std::ios::binary);
#endif

	// Check if the file is opened successfully
	if (!file.is_open()) {
		std::wcerr << L"Failed to open file: " << _path << std::endl;
		return false;
	}

	std::vector<uint8_t> data(std::istreambuf_iterator<char>(file), {});

	return { std::move(data) };
}

bool dev::SaveFile(const std::wstring& _path, const std::vector<uint8_t>& _data, const bool _override)
{
#if defined(_WIN32)
	std::ofstream file{_path, std::ios::binary | std::ios::ate};
#elif defined(__linux__)
	std::ofstream file{dev::StrWToStr(_path), std::ios::binary | std::ios::ate};
#endif

	if (!file)
	{
		dev::Log(L"Failed to init a file object: {}", _path);
		return false;	
	}
	else if ((!_override && std::filesystem::exists(_path)))
	{
		dev::Log(L"Error. Failed to save file. File is already exist: {}", _path);
		return false;
	}

	file.write(reinterpret_cast<const char*>(_data.data()), _data.size());
	return true;
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

auto dev::GetFilename(const std::wstring& _path)
->std::wstring
{
	std::filesystem::path p{ _path };
	return p.stem().wstring();
}

auto dev::GetExt(const std::wstring& _path)
->std::wstring
{
	std::filesystem::path p{ _path };
	return p.extension().wstring();
}

auto dev::GetDirStemExt(const std::wstring& _path)
-> std::tuple<std::wstring, std::wstring, std::wstring>
{
	std::filesystem::path p{ _path };
	return std::make_tuple(p.parent_path().wstring(), p.stem().wstring(), p.extension().wstring());
}
