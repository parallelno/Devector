// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "utils.h"
#include "utils/globals.h"
#include "utils/stringUtils.h"
//#include "utils/time.h"

#include <cstdarg>
#include <fstream>

void dev::RunApp(const std::string& _dir, const std::string& _appName) 
{
	if ( !IsFileExist(_dir + _appName) )
	{
		return;
	}
	const std::string command = "cd " + _dir + " && " + _appName;
	system(command.c_str());
}

void dev::ThreadSleep(double _seconds) 
{
	std::this_thread::sleep_for(std::chrono::milliseconds((long long)(_seconds * 1000.0)));
}

auto dev::LoadTextFile(const std::string& path)
	-> std::vector<std::string>
{	
	if (!IsFileExist(path)) {
		return {};
	}
	std::ifstream ifs(path);
	std::string fileStr = std::string((std::istreambuf_iterator<char>(ifs)),
		(std::istreambuf_iterator<char>()));

	return dev::Split(fileStr, '\n');
}

void dev::DeleteFiles(const std::string& _dir, const std::string& _mask)
{
	const std::string command = "del /Q " + _dir + _mask;
	system(command.c_str());
}