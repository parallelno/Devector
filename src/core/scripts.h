#pragma once

#include <string>
#include <unordered_map>

#include <lua.hpp>

#include "utils/json_utils.h"
#include "utils/types.h"
#include "core/script.h"
#include "core/cpu_i8080.h"
#include "core/memory.h"
#include "core/io.h"
#include "core/display.h"


namespace dev
{
	struct Scripts
	{
	public:
		using ScriptMap = std::unordered_map<dev::Id, Script>;

		Scripts();
		~Scripts();
		void Add(Script&& _bp);
		void Add(const nlohmann::json& _wpJ);
		void Del(const dev::Id _id);
		bool Check(const CpuI8080::State& _cpuState, const Memory::State& _memState,
			const IO::State& _ioState, const Display::State& _displayState);
		auto GetAll() -> const ScriptMap&;
		auto GetUpdates() -> const uint32_t;
		void Clear();

	private:
		void RegisterCppFunctions();
		bool RunScript(const std::string& script);

		ScriptMap m_scripts;
		uint32_t m_updates = 0; // counts number of updates
		lua_State* m_luaState;
		bool m_enabled = false;

		int sharedCounter = 42; // Example shared data in lua scripts (e.g., a counter)
	};
}