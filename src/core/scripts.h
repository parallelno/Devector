#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

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
		enum UIType {
			NONE = 0,
			TEXT,
			RECT,
		};
		struct UIItem{
			UIType type = NONE;
			float x = 0;
			float y = 0;
			float width = 0;
			float height = 0;
			std::string text = "";
			uint32_t color = 0xFFFFFFFF;
		};
		using UIReqs = std::unordered_map<dev::Id, UIItem>; // for UI rendering, this is used by Lua to request a render UI items in the UI thread
		using ScriptMap = std::unordered_map<dev::Id, Script>;

		Scripts();
		~Scripts();
		void Add(Script&& _script);
		void Add(const nlohmann::json& _scriptJ);
		auto Find(const dev::Id _id) -> const Script*;
		void Del(const dev::Id _id);
		bool Check(const CpuI8080::State* _cpuStateP, const Memory::State* _memStateP,
			const IO::State* _ioStateP, const Display::State* _displayStateP);
		auto GetAll() -> const ScriptMap&;
		auto GetUpdates() -> const uint32_t;
		void Clear();
		auto GetUIItems() -> const UIReqs
		{
			std::lock_guard<std::mutex> mlock(m_uiReqsMutex);
			UIReqs uiReqs;
			uiReqs.reserve(m_uiReqs.size());
			for (const auto& [id, item] : m_uiReqs)
			{
				uiReqs[id] = item;
			}
			return uiReqs;
		}

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

		UIReqs m_uiReqs;
		std::mutex m_uiReqsMutex;
	};
}