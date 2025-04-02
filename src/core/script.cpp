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

void dev::Script::Check(lua_State* _luaState)
{
	if (data.active) RunScript(_luaState);
}

void dev::Script::Reset()
{
}

void dev::Script::Print(bool _printCode) const
{
	if (_printCode){
		dev::Log("Script id: {}, comment: {}, active: {} \n code: \n{}",
			data.id, comment.c_str(), data.active, code.c_str());
	}
	else{
		dev::Log("Script id: {}, comment: {}, active: {}",
			data.id, comment.c_str(), data.active);
	}
}

void dev::Script::CompileScript(lua_State* _luaState)
{
	int status = luaL_loadstring(_luaState, code.c_str());
	if (status != LUA_OK) {
		dev::Log("Scripts: compilation error. Script id: {}, comment: {},\n error: {}", 
			data.id, comment, lua_tostring(_luaState, -1));
		lua_pop(_luaState, 1);
		data.active = false;
		return;
	}

	// Store the compiled function in the Lua registry and get a reference
	ref = luaL_ref(_luaState, LUA_REGISTRYINDEX);
}

void dev::Script::RunScript(lua_State* _luaState)
{
	if (ref == LUA_NOREF) {
		dev::Log("Scripts: Invalid script reference: {}. Script is disabled.", ref);
		data.active = false;
	}

	// Retrieve the compiled function from the registry
	lua_rawgeti(_luaState, LUA_REGISTRYINDEX, ref);

	// Execute the function
	int status = lua_pcall(_luaState, 0, LUA_MULTRET, 0);
	if (status != LUA_OK) {
		dev::Log("Script: Lua execution error: {}", lua_tostring(_luaState, -1));
		lua_pop(_luaState, 1);
		data.active = false;
	}
}