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
		void Add(Script&& _script);
		void Add(const nlohmann::json& _scriptJ);
		void Del(const dev::Id _id);
		bool Check(const CpuI8080::State* _cpuStateP, const Memory::State* _memStateP,
			const IO::State* _ioStateP, const Display::State* _displayStateP);
		auto GetAll() -> const ScriptMap&;
		auto GetUpdates() -> const uint32_t;
		void Clear();

	private:
		void RegisterCppFunctions();
		void CompileScript(Script& _script);
		void RunScript(int _scriptRef);

		ScriptMap m_scripts;
		uint32_t m_updates = 0; // counts number of updates
		lua_State* m_luaState;
		bool m_enabled = false;

		const CpuI8080::State* m_cpuStateP = nullptr;
		const Memory::State* m_memStateP = nullptr;
		const IO::State* m_ioStateP = nullptr;
		const Display::State* m_displayStateP = nullptr;
		bool m_break = false;
	};
}