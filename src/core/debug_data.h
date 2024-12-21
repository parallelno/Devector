#pragma once

#include <unordered_map>
#include <vector>
#include <limits.h>

#include "utils/types.h"
#include "utils/str_utils.h"
#include "core/hardware.h"

#include "core/breakpoints.h"
#include "core/watchpoints.h"

namespace dev
{
	class DebugData
	{
	public:
		using AddrLabels = std::vector<std::string>;
		using Labels = std::unordered_map<GlobalAddr, AddrLabels>;
		using Comments = std::unordered_map<GlobalAddr, std::string>;

		DebugData(Hardware& _hardware);

		auto GetComment(const Addr _addr) const -> const std::string*;
		void SetComment(const Addr _addr, const std::string& _comment);
		void DelComment(const Addr _addr);

		auto GetLabels(const Addr _addr) const -> const AddrLabels*;
		void SetLabels(const Addr _addr, const AddrLabels& _labels);

		auto GetConsts(const Addr _addr) const -> const AddrLabels*;
		void SetConsts(const Addr _addr, const AddrLabels& _labels);

		auto GetBreakpoints() -> Breakpoints* { return &m_breakpoints; };
		auto GetWatchpoints() -> Watchpoints* { return &m_watchpoints; };

		void LoadDebugData(const std::wstring& _path);
		void SaveDebugData();

		void Reset();

	private:
		Hardware& m_hardware;

		Labels m_labels;	// labels
		Labels m_consts;	// labels used as constants or they point to data
		Comments m_comments;

		Breakpoints m_breakpoints;
		Watchpoints m_watchpoints;

		std::wstring m_debugPath;
	};
}