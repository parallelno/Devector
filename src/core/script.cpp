#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>

#include "core/script.h"
#include "utils/str_utils.h"
#include "utils/utils.h"

dev::Script::Script(Data&& _data, const std::string& _code, const std::string& _comment)
	:
	data(std::move(_data)), code(_code), comment(_comment)
{}

void dev::Script::Update(Script&& _script)
{
	data = std::move(_script.data);
	code = std::move(_script.code);
	comment = std::move(_script.comment);
}

auto dev::Script::Check(const CpuI8080::State& _cpuState, const Memory::State& _memState,
	const IO::State& _ioState, const Display::State& _displayState)
->const bool
{
	if (!data.active) return false;

	return true;
}

void dev::Script::Reset()
{
}

void dev::Script::Print() const
{
	// std::printf("0x%05x, access: %s, cond: %s, value: 0x%04x, type: %s, len: %d, active: %d \n", 
	// 	data.globalAddr, GetAccessS(), GetConditionS(),
	// 	data.value, GetTypeS(), data.len, data.active);
}