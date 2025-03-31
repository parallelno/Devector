#include <string>

#include "core/scripts.h"
#include "utils/str_utils.h"
#include "utils/utils.h"

dev::Scripts::Scripts()
{
	m_luaState = luaL_newstate();
	if (!m_luaState) {
		dev::Log("Failed to create lua state. Script support is disabled.");
		return;
	}

	luaL_openlibs(m_luaState);  // Load standard Lua libraries
	RegisterCppFunctions();

	// TODO: test
	std::string script = "print(\"Script 1: Adding 5 + 3 = \" .. add(5, 3))\n"
						"print(\"Script 1: Counter = \" .. getCounter())\n"
						"setCounter(100)\n";

	RunScript(script);

	m_enabled = true;
}

dev::Scripts::~Scripts()
{
	if (m_luaState){
		lua_close(m_luaState);
	}
}

void dev::Scripts::RegisterCppFunctions()
{
	// Register a function to add two numbers
	lua_pushcfunction(m_luaState, [](lua_State* m_luaState) -> int {
		double a = luaL_checknumber(m_luaState, 1);
		double b = luaL_checknumber(m_luaState, 2);
		lua_pushnumber(m_luaState, a + b);
		return 1;
	});
	lua_setglobal(m_luaState, "add");

	// Register a function to get shared data (e.g., counter)
	lua_pushcfunction(m_luaState, [](lua_State* m_luaState) -> int {
		Scripts* scripts = static_cast<Scripts*>(lua_touserdata(m_luaState, lua_upvalueindex(1)));
		lua_pushinteger(m_luaState, scripts->sharedCounter);
		return 1;
	});
	lua_pushlightuserdata(m_luaState, this);  // Pass 'this' as upvalue
	lua_pushcclosure(m_luaState, lua_tocfunction(m_luaState, -2), 1);
	lua_setglobal(m_luaState, "getCounter");

	// Register a function to set shared data
	lua_pushcfunction(m_luaState, [](lua_State* m_luaState) -> int {
		Scripts* scripts = static_cast<Scripts*>(lua_touserdata(m_luaState, lua_upvalueindex(1)));
		scripts->sharedCounter = luaL_checkinteger(m_luaState, 1);
		return 0;
	});
	lua_pushlightuserdata(m_luaState, this);
	lua_pushcclosure(m_luaState, lua_tocfunction(m_luaState, -2), 1);
	lua_setglobal(m_luaState, "setCounter");
}

bool dev::Scripts::RunScript(const std::string& script)
{
    int status = luaL_loadstring(m_luaState, script.c_str());
    if (status != LUA_OK) {
        std::cerr << "Lua load error: " << lua_tostring(m_luaState, -1) << std::endl;
        lua_pop(m_luaState, 1);
        return false;
    }

    status = lua_pcall(m_luaState, 0, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        std::cerr << "Lua execution error: " << lua_tostring(m_luaState, -1) << std::endl;
        lua_pop(m_luaState, 1);
        return false;
    }
    return true;
}

// Hardware thread
void dev::Scripts::Clear()
{
	m_scripts.clear();
	m_updates++;
}

// Hardware thread
void dev::Scripts::Add(Script&& _script)
{
	if (!m_enabled) return;

	m_updates++;

	auto scriptI = m_scripts.find(_script.data.id);
	if (scriptI != m_scripts.end())
	{
		scriptI->second.Update(std::move(_script));
		return;
	}
	
	m_scripts.emplace(_script.data.id, std::move(_script));
}

void dev::Scripts::Add(const nlohmann::json& _scriptJ)
{
	if (!m_enabled) return;

	m_updates++;

	Script::Data scriptData {_scriptJ};
	Script script{ std::move(scriptData), _scriptJ["comment"] };

	auto scriptI = m_scripts.find(script.data.id);
	if (scriptI != m_scripts.end())
	{
		scriptI->second.Update(std::move(script));
		return;
	}
	
	m_scripts.emplace(script.data.id, std::move(script));
}

// Hardware thread
void dev::Scripts::Del(const dev::Id _id)
{
	if (!m_enabled) return;

	m_updates++;
	auto bpI = m_scripts.find(_id);
	if (bpI != m_scripts.end())
	{
		m_scripts.erase(bpI);
	}
}

// Hardware thread
bool dev::Scripts::Check(const CpuI8080::State& _cpuState, const Memory::State& _memState,
	const IO::State& _ioState, const Display::State& _displayState)
{
	if (!m_enabled) return false;

	auto scriptI = std::find_if(m_scripts.begin(), m_scripts.end(),
		[_cpuState, _memState, _ioState, _displayState](ScriptMap::value_type& pair)
		{
			return pair.second.Check(_cpuState, _memState, _ioState, _displayState);
		});
	
	return scriptI != m_scripts.end();
}

// Hardware thread
auto dev::Scripts::GetAll()
-> const ScriptMap&
{
	return m_scripts;
}

// Hardware thread
auto dev::Scripts::GetUpdates()
-> const uint32_t
{
	return m_updates;
}