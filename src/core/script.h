#pragma once

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <format>

#include <lua.hpp>

#include "utils/types.h"
#include "utils/str_utils.h"
#include "utils/utils.h"
#include "core/cpu_i8080.h"
#include "core/memory.h"
#include "core/io.h"
#include "core/display.h"
#include "utils/json_utils.h"

namespace dev
{
	static Id scriptId = 0;

	struct Script
	{
		Script(const Id _id, const bool _active, const std::string& _code, const std::string& _comment = "");
		Script() : Script(scriptId++, true, "", "") {}
		Script(const nlohmann::json& _scriptJ);
		void CompileScript(lua_State* _luaState);
		void RunScript(lua_State* _luaState);
		void Update(Script&& _script);
		void Check(lua_State* _luaState);
		auto GetComment() const -> const std::string& { return comment;  };
		void Reset();
		void Print(bool _printCode = false) const;
		auto ToStr(bool _printId, bool _printComment) const -> const std::string;
		auto GetCode(const int _lines) const -> const std::string;
		auto ToJson() const -> nlohmann::json
		{
			return {
				{"id", id},
				{"active", active},
				{"code", code},
				{"comment", comment},
			};
		};

		Id id;
		bool active;
		std::string code;
		std::string comment;
		int ref = LUA_NOREF;
	};
}