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
		using LabelList = std::vector<std::string>;
		using Labels = std::unordered_map<GlobalAddr, LabelList>;
		using Comments = std::unordered_map<GlobalAddr, std::string>;

		using UpdateId = int;
		
		using SymbolAddrList = std::vector<std::tuple<std::string, GlobalAddr, std::string>>; // symbol, addr, addrS

		DebugData(Hardware& _hardware);

		auto GetComment(const Addr _addr) const -> const std::string*;
		void SetComment(const Addr _addr, const std::string& _comment);
		void DelComment(const Addr _addr);
		void GetFilteredComments(SymbolAddrList& _out, const std::string& _filter = "") const;

		auto GetLabels(const Addr _addr) const -> const LabelList*;
		void SetLabels(const Addr _addr, const LabelList& _labels);
		void GetFilteredLabels(SymbolAddrList& _out, const std::string& _filter = "") const;

		auto GetConsts(const Addr _addr) const -> const LabelList*;
		void SetConsts(const Addr _addr, const LabelList& _labels);
		void GetFilteredConsts(SymbolAddrList& _out, const std::string& _filter = "") const;

		auto GetCommentsUpdates() const -> UpdateId { return m_commentsUpdates; };
		auto GetLabelsUpdates() const -> UpdateId { return m_labelsUpdates; };
		auto GetConstsUpdates() const -> UpdateId { return m_constsUpdates; };

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

		UpdateId m_labelsUpdates = 0;
		UpdateId m_constsUpdates = 0;
		UpdateId m_commentsUpdates = 0;
	};
}