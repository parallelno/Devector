#pragma once

#include <unordered_map>
#include <vector>
#include <limits.h>
#include <format>

#include "utils/types.h"
#include "utils/str_utils.h"
#include "core/hardware.h"

#include "core/breakpoints.h"
#include "core/watchpoints.h"
#include "core/code_perf.h"
#include "core/memory_edit.h"
#include "core/scripts.h"
#include "utils/json_utils.h"

namespace dev
{
	class DebugData
	{
	public:
		using LabelList = std::vector<std::string>;
		using Labels = std::unordered_map<GlobalAddr, LabelList>;
		using Comments = std::unordered_map<GlobalAddr, std::string>;

		using UpdateId = int;

		using FilteredElements = std::vector<std::tuple<std::string, GlobalAddr, std::string>>; // name, addr, addrS

		// injects the value into the memory while loading

		using MemoryEdits = std::unordered_map<GlobalAddr, MemoryEdit>;
		using CodePerfs = std::unordered_map<Addr, CodePerf>;

		DebugData(Hardware& _hardware);

		auto GetLabels(const Addr _addr) const -> std::optional<LabelList>;
		auto GetLabelAddr(const std::string& _label) -> int;
		void SetLabels(const Addr _addr, const LabelList& _labels);
		void AddLabel(const Addr _addr, const std::string& _label);
		void DelLabel(const Addr _addr, const std::string& _label);
		void DelLabels(const Addr _addr);
		void DelAllLabels();
		void RenameLabel(const Addr _addr, const std::string& _oldLabel, const std::string& _newLabel);
		void GetFilteredLabels(FilteredElements& _out, const std::string& _filter = "") const;

		auto GetConsts(const Addr _addr) const -> const LabelList*;
		void SetConsts(const Addr _addr, const LabelList& _consts);
		void AddConst(const Addr _addr, const std::string& _const);
		void DelConst(const Addr _addr, const std::string& _const);
		void DelConsts(const Addr _addr);
		void DelAllConsts();
		void RenameConst(const Addr _addr, const std::string& _oldConst, const std::string& _newConst);
		void GetFilteredConsts(FilteredElements& _out, const std::string& _filter = "") const;

		auto GetComment(const Addr _addr) const -> const std::string*;
		void SetComment(const Addr _addr, const std::string& _comment);
		void DelComment(const Addr _addr);
		void DelAllComments();
		void GetFilteredComments(FilteredElements& _out, const std::string& _filter = "") const;

		auto GetMemoryEdit(const Addr _addr) const -> const MemoryEdit*;
		void SetMemoryEdit(const MemoryEdit& _edit);
		void DelMemoryEdit(const Addr _addr);
		void DelAllMemoryEdits();
		void GetFilteredMemoryEdits(FilteredElements& _out, const std::string& _filter = "") const;

		auto GetCodePerf(const Addr _addr) const -> const CodePerf*;
		void SetCodePerf(const CodePerf& _codePerf);
		void DelCodePerf(const Addr _addr);
		void DelAllCodePerfs();
		void GetFilteredCodePerfs(FilteredElements& _out, const std::string& _filter = "") const;
		auto CheckCodePerfs(const Addr _addrStart, const uint64_t _cc) -> bool;

		void GetFilteredScripts(FilteredElements& _out, const std::string& _filter = "");

		auto GetCommentsUpdates() const -> UpdateId { return m_commentsUpdates; };
		auto GetLabelsUpdates() const -> UpdateId { return m_labelsUpdates; };
		auto GetConstsUpdates() const -> UpdateId { return m_constsUpdates; };
		auto GetEditsUpdates() const -> UpdateId { return m_editsUpdates; };
		auto GetCodePerfsUpdates() const -> UpdateId { return m_codePerfsUpdates; };

		auto GetBreakpoints() -> Breakpoints& { return m_breakpoints; };
		auto GetWatchpoints() -> Watchpoints& { return m_watchpoints; };
		auto GetScripts() -> Scripts& { return m_scripts; };

		void LoadDebugData(const std::string& _path);
		void SaveDebugData();
		auto GetPath() const -> const std::string& { return m_debugPath; };

		void Reset();

	private:

		Hardware& m_hardware;

		Labels m_labels;	// labels
		Labels m_consts;	// labels used as constants or they point to data
		Comments m_comments;
		MemoryEdits m_memoryEdits; // code/data modifications
		CodePerfs m_codePerfs;
		Breakpoints m_breakpoints;
		Watchpoints m_watchpoints;
		Scripts m_scripts;

		std::string m_debugPath;

		UpdateId m_labelsUpdates = 0;
		UpdateId m_constsUpdates = 0;
		UpdateId m_commentsUpdates = 0;
		UpdateId m_editsUpdates = 0;
		UpdateId m_codePerfsUpdates = 0;
	};
}