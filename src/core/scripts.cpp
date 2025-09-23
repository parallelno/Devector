#include <string>

#include <type_traits>

#include "core/scripts.h"
#include "utils/str_utils.h"
#include "utils/utils.h"

dev::Scripts::Scripts(LabelAddrFunc _getLabelAddrFunc)
	: GetLabelAddrFunc(_getLabelAddrFunc)
{
	m_luaState = luaL_newstate();
	if (!m_luaState) {
		dev::Log("Scripts: Failed to create lua state. Script support is disabled.");
		return;
	}

	luaL_openlibs(m_luaState);  // Load standard Lua libraries
	RegisterCppFunctions();

	m_enabled = true;
}

dev::Scripts::~Scripts()
{
	Clear();
	if (m_luaState){
		lua_close(m_luaState);
	}
}

// Helper struct with template specializations for type handling
struct LuaGetter {
	template<typename T>
	static int pushValue(lua_State* state, T value) {
		return 0;
	}
};

// Specializations for supported types
template<> inline int LuaGetter::pushValue<bool>(
		lua_State* state, bool value)
{
	lua_pushboolean(state, value);
	return 1;
}
template<> inline int LuaGetter::pushValue<uint8_t>(
		lua_State* state, uint8_t value)
{
	lua_pushinteger(state, static_cast<lua_Integer>(value));
	return 1;
}
template<> inline int LuaGetter::pushValue<uint16_t>(
		lua_State* state, uint16_t value)
{
	lua_pushinteger(state, static_cast<lua_Integer>(value));
	return 1;
}
template<> inline int LuaGetter::pushValue<int>(
		lua_State* state, int value)
{
	lua_pushinteger(state, static_cast<lua_Integer>(value));
	return 1;
}
template<> inline int LuaGetter::pushValue<uint64_t>(
		lua_State* state, uint64_t value)
{
	lua_pushinteger(state, static_cast<lua_Integer>(value));
	return 1;
}
template<> inline int LuaGetter::pushValue<double>(
		lua_State* state, double value)
{
	lua_pushnumber(state, value);
	return 1;
}
template<> inline int LuaGetter::pushValue<const char*>(
	lua_State* state, const char* value)
{
	lua_pushstring(state, value);
	return 1;
}

#define REGISTER_STRUCT_FIELD_GETTER(LUA_STATE, STRUCT_PTR, FIELD_PATH, FUNC_NAME) \
	do { \
		lua_CFunction getterFunc = [](lua_State* state) -> int { \
			using StructType = std::remove_pointer_t<decltype(STRUCT_PTR)>; \
			auto* structPtr = static_cast<StructType**>(lua_touserdata(state, lua_upvalueindex(1))); \
			LuaGetter::pushValue(state, (*structPtr)->FIELD_PATH); \
			return 1; \
		}; \
		lua_pushlightuserdata(LUA_STATE, (void*)(&STRUCT_PTR)); \
		lua_pushcclosure(LUA_STATE, getterFunc, 1); \
		lua_setglobal(LUA_STATE, FUNC_NAME); \
	} while(0)

void dev::Scripts::RegisterCppFunctions()
{
	// Register a Break function
	lua_CFunction breakFunc = [](lua_State* _luaState) -> int {
		Scripts* scripts = static_cast<Scripts*>(lua_touserdata(_luaState, lua_upvalueindex(1)));
		scripts->m_break = true;
		return 0;
	};
	lua_pushcfunction(m_luaState, breakFunc);
	lua_pushlightuserdata(m_luaState, this);
	lua_pushcclosure(m_luaState, breakFunc, 1);
	lua_setglobal(m_luaState, "Break");

	// Register CPU state getters
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, cc, "GetCC");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.pc.word, "GetPC");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.sp.word, "GetSP");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.psw.af.word, "GetPSW");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.bc.word, "GetBC");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.de.word, "GetDE");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.hl.word, "GetHL");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.psw.a, "GetA");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.psw.af.l, "GetF");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.bc.h, "GetB");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.bc.l, "GetC");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.de.h, "GetD");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.de.l, "GetE");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.hl.h, "GetH");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.hl.l, "GetL");

	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.psw.s, "GetFlagS");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.psw.z, "GetFlagZ");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.psw.ac, "GetFlagAC");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.psw.p, "GetFlagP");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, regs.psw.c, "GetFlagC");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, ints.inte, "GetINTE");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, ints.iff, "GetIFF");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, ints.hlta, "GetHLTA");
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_cpuStateP, ints.mc, "GetMachineCycles");

	// Register Memory state getters
	REGISTER_STRUCT_FIELD_GETTER(m_luaState, m_memStateP, debug.instr.opcode, "GetOpcode");

	lua_CFunction getterByteGlobal = [](lua_State* state) -> int {
		using StructType = std::remove_pointer_t<decltype(m_memStateP)>;
		auto* structPtr = static_cast<StructType**>(
			lua_touserdata(state, lua_upvalueindex(1)));

		int globalAddr = luaL_checkinteger(state, 1);
		int val = (*structPtr)->ramP->at(globalAddr);
		lua_pushinteger(state, val);
		return 1;
	};
	lua_pushlightuserdata(m_luaState, (void*)(&m_memStateP));
	lua_pushcclosure(m_luaState, getterByteGlobal, 1);
	lua_setglobal(m_luaState, "GetByteGlobal");

	// Get label addr
	lua_CFunction getterLabelAddr = [](lua_State* state) -> int {
		using StructType = std::remove_pointer_t<decltype(m_memStateP)>;

		auto* scriptsP = static_cast<Scripts*>(
			lua_touserdata(state, lua_upvalueindex(1)));

		const char* label = luaL_checkstring(state, 1);
		if (!label) {
			luaL_error(state, "GetLabelAddr: label argument must be a string");
			return 0;
		}

		if (scriptsP) {
			int addr = scriptsP->GetLabelAddrFunc(label);
			lua_pushinteger(state, addr);
		}

		return 1;
	};
	lua_pushlightuserdata(m_luaState, (void*)(this));
	lua_pushcclosure(m_luaState, getterLabelAddr, 1);
	lua_setglobal(m_luaState, "GetLabelAddr");

	// DrawText
	lua_CFunction drawTextFunc = [](lua_State* state) -> int
	{
		auto paramNum = lua_gettop(state);
		uint32_t color = 0xFFFFFFFF;
		bool vectorScreenCoords = true;

		if (paramNum < 4 || paramNum > 6) {
			luaL_error(state,
				"DrawText: wrong number of parameters: "
				"(id, text, x, y, <color=0xFFFFFFFF>, "
				"<vectorScreenCoords=true>)");
			return 0;
		}

		int id = luaL_checkinteger(state, 1);
		const char* textCStr = luaL_checkstring(state, 2);
		float x = luaL_checknumber(state, 3);
		float y = luaL_checknumber(state, 4);
		if (paramNum >= 5) {
			color = static_cast<uint32_t>(luaL_checkinteger(state, 5));
		}
		if (paramNum >= 6) {
			vectorScreenCoords = lua_toboolean(state, 6);
		}

		if (!textCStr) {
			luaL_error(state, "DrawText: text argument must be a string");
		}

		auto* scriptsP = static_cast<Scripts*>(
			lua_touserdata(state, lua_upvalueindex(1)));
		if (scriptsP) {
			auto& uiReqs = scriptsP->m_uiReqs;
			auto& lock = scriptsP->m_uiReqsMutex;

			std::lock_guard<std::mutex> lockGuard(lock);
			uiReqs[id] = UIItem{
				Scripts::UIType::TEXT,
				x, y, 0, 0,
				textCStr, color, vectorScreenCoords};
		}
		return 0;
	};
	lua_pushlightuserdata(m_luaState, (void*)(this));
	lua_pushcclosure(m_luaState, drawTextFunc, 1);
	lua_setglobal(m_luaState, "DrawText");

	// DrawRect
	lua_CFunction drawRectFunc = [](lua_State* state) -> int
	{
		auto paramNum = lua_gettop(state);
		uint32_t color = 0xFFFFFFFF;
		bool vectorScreenCoords = true;

		if (paramNum < 5 || paramNum > 7) {
			luaL_error(state,
				"DrawRect: wrong number of parameters: "
				"(id, x, y, width, height, "
				"<color=0xFFFFFFFF>, <vectorScreenCoords=true>)");
			return 0;
		}

		int id = luaL_checkinteger(state, 1);
		float x = luaL_checknumber(state, 2);
		float y = luaL_checknumber(state, 3);
		float width = luaL_checknumber(state, 4);
		float height = luaL_checknumber(state, 5);
		if (paramNum >= 6) {
			color = static_cast<uint32_t>(luaL_checkinteger(state, 6));
		}
		if (paramNum >= 7) {
			vectorScreenCoords = lua_toboolean(state, 7);
		}

		auto* scriptsP = static_cast<Scripts*>(
			lua_touserdata(state, lua_upvalueindex(1)));

		if (scriptsP) {
			auto& uiReqs = scriptsP->m_uiReqs;
			auto& lock = scriptsP->m_uiReqsMutex;

			std::lock_guard<std::mutex> lockGuard(lock);
			uiReqs[id] = UIItem{
				Scripts::UIType::RECT,
				x, y, width, height,
				"", color, vectorScreenCoords};
		}
		return 0;
	};
	lua_pushlightuserdata(m_luaState, (void*)(this));
	lua_pushcclosure(m_luaState, drawRectFunc, 1);
	lua_setglobal(m_luaState, "DrawRect");

	// DrawRectFilled
	lua_CFunction drawRectFilledFunc = [](lua_State* state) -> int
	{
		auto paramNum = lua_gettop(state);
		uint32_t color = 0xFFFFFFFF;
		bool vectorScreenCoords = true;

		if (paramNum < 5 || paramNum > 7) {
			luaL_error(state,
				"DrawRectFilled: wrong number of parameters: "
				"(id, x, y, width, height, "
				"<color=0xFFFFFFFF>, <vectorScreenCoords=true>)");
			return 0;
		}

		int id = luaL_checkinteger(state, 1);
		float x = luaL_checknumber(state, 2);
		float y = luaL_checknumber(state, 3);
		float width = luaL_checknumber(state, 4);
		float height = luaL_checknumber(state, 5);
		if (paramNum >= 6) {
			color = static_cast<uint32_t>(luaL_checkinteger(state, 6));
		}
		if (paramNum >= 7) {
			vectorScreenCoords = lua_toboolean(state, 7);
		}

		auto* scriptsP = static_cast<Scripts*>(lua_touserdata(state, lua_upvalueindex(1)));
		if (scriptsP) {
			auto& uiReqs = scriptsP->m_uiReqs;
			auto& lock = scriptsP->m_uiReqsMutex;

			std::lock_guard<std::mutex> lockGuard(lock);
			uiReqs[id] = UIItem{
				Scripts::UIType::RECT_FILLED,
				x, y, width, height, "", color, vectorScreenCoords};
		}
		return 0;
	};
	lua_pushlightuserdata(m_luaState, (void*)(this));
	lua_pushcclosure(m_luaState, drawRectFilledFunc, 1);
	lua_setglobal(m_luaState, "DrawRectFilled");

}

// Hardware thread
void dev::Scripts::Clear()
{
	for (auto& [id, script] : m_scripts)
	{
		luaL_unref(m_luaState, LUA_REGISTRYINDEX, script.ref);
	}
	m_scripts.clear();
	m_updates++;
}

// Hardware thread
void dev::Scripts::Add(Script&& _script)
{
	if (!m_enabled) return;

	m_updates++;

	auto scriptI = m_scripts.find(_script.id);
	if (scriptI != m_scripts.end())
	{
		scriptI->second.Update(std::move(_script));
		_script.CompileScript(m_luaState);
		return;
	}

	_script.CompileScript(m_luaState);
	m_scripts.emplace(_script.id, std::move(_script));
}

void dev::Scripts::Add(const nlohmann::json& _scriptJ)
{
	if (!m_enabled) return;

	m_updates++;

	auto id = _scriptJ["id"].get<Id>();
	auto scriptI = m_scripts.emplace(id, Script{}).first;
	scriptI->second.Update(_scriptJ);
	scriptI->second.CompileScript(m_luaState);
}

auto dev::Scripts::Find(const dev::Id _id)
-> const Script*
{
	auto scriptI = m_scripts.find(_id);
	return scriptI != m_scripts.end() ? &scriptI->second : nullptr;
}

// Hardware thread
void dev::Scripts::Del(const dev::Id _id)
{
	if (!m_enabled) return;

	m_updates++;
	auto scriptI = m_scripts.find(_id);
	if (scriptI != m_scripts.end())
	{
		luaL_unref(m_luaState, LUA_REGISTRYINDEX, scriptI->second.ref);
		m_scripts.erase(scriptI);
	}
}

// Hardware thread
bool dev::Scripts::Check(
	const CpuI8080::State* _cpuStateP,
	const Memory::State* _memStateP,
	const IO::State* _ioStateP,
	const Display::State* _displayStateP)
{
	if (!m_enabled) return false;

	// TODO: rework it to not assign every cpu tick
	m_cpuStateP = _cpuStateP;
	m_memStateP = _memStateP;
	m_ioStateP = _ioStateP;
	m_displayStateP = _displayStateP;
	// TODO: end of rework


	bool total_break = false;
	for (auto& [id, script] : m_scripts)
	{
		m_break = false;
		script.Check(m_luaState);
		total_break |= m_break;

		if (m_break) {
			dev::Log("Script break");
			script.Print();
		}
	}

	return total_break;
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