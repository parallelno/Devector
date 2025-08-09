#include <cstdarg>
#include <fstream>
#include <iostream>
#include <thread>

#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
#endif

#include "utils/utils.h"
#include "utils/consts.h"
#include "utils/str_utils.h"
#include "utils/result.h"

void dev::RunApp(const std::string& _dir, const std::string& _appName)
{
	if ( !IsFileExist(_dir + _appName) )
	{
		return;
	}
	const std::string command = "cd " + _dir + " && " + _appName;
	auto suppress_warning = system(command.c_str());
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
	auto suppress_warning = system(command);
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

auto dev::LoadTextFile(const std::string& _path)
-> std::string
{
	if (!IsFileExist(_path)) {
		return {};
	}

	std::ifstream ifs(_path);

	std::string data = std::string(std::istreambuf_iterator<char>(ifs),
		std::istreambuf_iterator<char>());

	return data;
}

auto dev::LoadFile(const std::string& _path)
-> Result<std::vector<uint8_t>>
{
	if (!IsFileExist(_path)) {
		return {};
	}

	// Open the file in binary mode
	std::ifstream file(_path, std::ios::binary);

	// Check if the file is opened successfully
	if (!file.is_open()) {
		std::cerr << "Failed to open file: " << _path << std::endl;
		return {};
	}

	std::vector<uint8_t> data(std::istreambuf_iterator<char>(file), {});

	return { std::move(data) };
}

bool dev::SaveFile(const std::string& _path, const std::vector<uint8_t>& _data, const bool _override)
{
	std::ofstream file{_path, std::ios::binary | std::ios::ate};

	if (!file)
	{
		dev::Log("Failed to init a file object: {}", _path);
		return false;
	}
	else if ((!_override && std::filesystem::exists(_path)))
	{
		dev::Log("Error. Failed to save file. File is already exist: {}", _path);
		return false;
	}

	file.write(reinterpret_cast<const char*>(_data.data()), _data.size());
	return true;
}

void dev::DeleteFiles(const std::string& _dir, const std::string& _mask)
{
	const std::string command = "del /Q " + _dir + _mask;
	auto suppress_warning = system(command.c_str());
}

size_t dev::GetFileSize(const std::string& _path)
{
	std::filesystem::path p{ _path };
	return std::filesystem::file_size(p);
}

auto dev::GetDir(const std::string& _path)
->std::string
{
	std::filesystem::path p{ _path };
	return p.parent_path().string();
}

auto dev::GetFilename(const std::string& _path)
->std::string
{
	std::filesystem::path p{ _path };
	return p.stem().string();
}

auto dev::GetExt(const std::string& _path)
->std::string
{
	std::filesystem::path p{ _path };
	return p.extension().string();
}

auto dev::GetDirStemExt(const std::string& _path)
-> std::tuple<std::string, std::string, std::string>
{
	std::filesystem::path p{ _path };
	return std::make_tuple(p.parent_path().string(), p.stem().string(), p.extension().string());
}

auto dev::GetExecutableDir()
-> std::string
{
	std::string path;
#ifdef _WIN32
	char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    path = buffer;
#else
	char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer)-1);
    if (len != -1) {
        buffer[len] = '\0';
        path = std::string(buffer);
    }
#endif
	return dev::GetDir(path) + "/";
}