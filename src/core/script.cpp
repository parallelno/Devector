#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>

#include "core/script.h"
#include "utils/str_utils.h"
#include "utils/utils.h"

dev::Script::Script(const Id _id, const bool _active, const std::string& _code, const std::string& _comment)
	:
	id(_id), active(_active), code(_code), comment(_comment)
{}

dev::Script::Script(const nlohmann::json& _scriptJ)
	: id(_scriptJ.value("id", scriptId++)),
	  active(_scriptJ.value("active", true)),
	  code(_scriptJ.value("code", "")),
	  comment(_scriptJ.value("comment", ""))
{}

void dev::Script::Update(Script&& _script)
{
	id = _script.id;
	active = _script.active;
	code = _script.code;
	comment = _script.comment;
}

void dev::Script::Check(lua_State* _luaState)
{
	if (active) RunScript(_luaState);
}

void dev::Script::Reset()
{
}

void dev::Script::Print(bool _printCode) const
{
	if (_printCode){
		dev::Log("Script id: {}, comment: {}, active: {} \n code: \n{}",
			id, comment.c_str(), active, code.c_str());
	}
	else{
		dev::Log("Script id: {}, comment: {}, active: {}",
			id, comment.c_str(), active);
	}
}

auto dev::Script::ToStr(bool _printId, bool _printComment) const 
-> const std::string
{
	std::string out;
	if (_printId) out += std::format("Id: {}, ", id);
	if (_printComment) out += std::format("Comment: {}, ", comment);
	out += std::format("Active: {}, code {}", active ? "true" : "false", code);
	return out;
}

auto dev::Script::GetCode(const int _lines) const -> const std::string
{
	if (_lines <= 0) return code;

	std::string out;
	size_t line = 0;
	std::istringstream iss(code);
	std::string lineS;

	while (std::getline(iss, lineS) && line < _lines) {
		out += lineS + "\n";
		line++;
	}
	const auto total_lines = std::count(std::begin(code), std::end(code), '\n');
	if (line < total_lines) out += "...\n";

	return out;
}

void dev::Script::CompileScript(lua_State* _luaState)
{
	int status = luaL_loadstring(_luaState, code.c_str());
	if (status != LUA_OK) {
		dev::Log("Scripts: compilation error. Script id: {}, comment: {},\n error: {}", 
			id, comment, lua_tostring(_luaState, -1));
		lua_pop(_luaState, 1);
		active = false;
		return;
	}

	// Store the compiled function in the Lua registry and get a reference
	ref = luaL_ref(_luaState, LUA_REGISTRYINDEX);
}

void dev::Script::RunScript(lua_State* _luaState)
{
	if (ref == LUA_NOREF) {
		dev::Log("Scripts: Invalid script reference: {}. Script is disabled.", ref);
		active = false;
	}

	// Retrieve the compiled function from the registry
	lua_rawgeti(_luaState, LUA_REGISTRYINDEX, ref);

	// Execute the function
	int status = lua_pcall(_luaState, 0, LUA_MULTRET, 0);
	if (status != LUA_OK) {
		dev::Log("Script: Lua execution error: {}", lua_tostring(_luaState, -1));
		lua_pop(_luaState, 1);
		active = false;
	}
}