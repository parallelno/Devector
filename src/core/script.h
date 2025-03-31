#pragma once

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <format>

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

#pragma pack(push, 1)
		union Data {
			struct {
				Id id;
				bool active		: 1;
			};
			struct {
				uint64_t data0;
			};

			Data(const Id _id, const bool _active = true):
				id(_id == -1 ? scriptId++ : _id), active(_active)
			{};
			Data(const uint64_t _data0)
				:
				data0(_data0)
			{}
			Data(const nlohmann::json& _scriptJ) : 
				Data(_scriptJ["id"],
					_scriptJ["active"])
			{};
		};
#pragma pack(pop)

		Script(Data&& _data, const std::string& _code, const std::string& _comment = "");

		void Update(Script&& _script);

		auto Check(const CpuI8080::State& _cpuState, const Memory::State& _memState,
			const IO::State& _ioState, const Display::State& _displayState) -> const bool;
		auto GetComment() const -> const std::string& { return comment;  };
		void Reset();
		void Print() const;
		auto ToJson() const -> nlohmann::json
		{
			return {
				{"id", data.id},
				{"active", data.active},
				{"code", code},
				{"comment", comment},
			};
		};

		Data data;
		std::string code;
		std::string comment;
	};
}