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
		struct MemoryEdit
		{
			std::string comment;
			GlobalAddr globalAddr = 0;
			uint8_t value = 0;
			bool readonly = true; // if true, memory is not modified
			bool active = true;

			auto AddrToStr() const -> std::string { return std::format("0x{:06x}: 0x{:02x} {}, {}", globalAddr, value, readonly ? "read-only" : "", active ? "active" : "not active"); }
			void Erase()
			{
				comment.clear();
				globalAddr = 0;
				value = 0;
				readonly = true;
				active = true;
			}

			auto ToJson() const -> nlohmann::json
			{
				return {
					{"comment", comment},
					{"globalAddr", std::format("0x{:06X}", globalAddr)},
					{"value", std::format("0x{:02X}", value)},
					{"readonly", readonly},
					{"active", active}
				};
			}

			MemoryEdit() = default;

			MemoryEdit(const nlohmann::json& _json)
				:
				comment(_json["comment"].get<std::string>()),
				globalAddr(dev::StrHexToInt(_json["globalAddr"].get<std::string>().c_str())),
				value(dev::StrHexToInt(_json["value"].get<std::string>().c_str())),
				readonly(_json["readonly"].get<bool>()),
				active(_json["active"].get<bool>())
			{}

			MemoryEdit(GlobalAddr _globalAddr, uint8_t _value, const std::string& _comment = "", bool _readonly = true, bool _active = true)
				:
				comment(_comment),
				globalAddr(_globalAddr),
				value(_value),
				readonly(_readonly),
				active(_active) {}
		};

		using MemoryEdits = std::unordered_map<GlobalAddr, MemoryEdit>;

		struct CodePerf
		{
			std::string label;
			Addr addrStart = 0;
			Addr addrEnd = 0x100;
			double averageCcDiff = 0.0f; // average cc (Cpu CLock) difference between it entered the addrStart and exited addrEnd
			int64_t tests = 0; // how many testes executed
			int64_t cc = 0; // the current perf test cc
			bool active = true;

			auto AddrToStr() const -> std::string {
				return std::format("0x{:06x}-0x{:06x}: {}, average cc: {}, tests: {}",
					addrStart, addrEnd,
					active ? "active" : "not active",
					static_cast<int>(std::round(averageCcDiff)),
					tests);
			}

			void Erase()
			{
				label.clear();
				addrStart = 0;
				addrEnd = 0x100;
				averageCcDiff = 0;
				tests = 0;
				cc = 0;
				active = true;
			}

			void CheckPerf(const Addr _addr, const uint64_t _cc)
			{
				if (!active || (addrStart != _addr && cc == 0)) return;

				if (addrStart == _addr){
					cc = _cc;
				}
				else
				if (addrEnd == _addr)
				{
						tests++;
						auto weight = 1.0 / tests;
						int64_t ccDiff = _cc - cc;
						averageCcDiff += (ccDiff - averageCcDiff) * weight;
						cc = 0;
				}
			}

			CodePerf() = default;

			CodePerf(const nlohmann::json& _json)
				:
				label(_json["label"].get<std::string>()),
				addrStart(dev::StrHexToInt(_json["addrStart"].get<std::string>().c_str())),
				addrEnd(dev::StrHexToInt(_json["addrEnd"].get<std::string>().c_str())),
				active(_json["active"].get<bool>())
			{}

			auto ToJson() const -> nlohmann::json
			{
				return {
					{"label", label},
					{"addrStart", std::format("0x{:04X}", addrStart)},
					{"addrEnd", std::format("0x{:04X}", addrEnd)},
					{"active", active}
				};
			}
		};
		using CodePerfs = std::unordered_map<Addr, CodePerf>;

		DebugData(Hardware& _hardware);

		auto GetLabels(const Addr _addr) const -> const LabelList*;
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

		auto GetCommentsUpdates() const -> UpdateId { return m_commentsUpdates; };
		auto GetLabelsUpdates() const -> UpdateId { return m_labelsUpdates; };
		auto GetConstsUpdates() const -> UpdateId { return m_constsUpdates; };
		auto GetEditsUpdates() const -> UpdateId { return m_editsUpdates; };
		auto GetCodePerfsUpdates() const -> UpdateId { return m_codePerfsUpdates; };

		auto GetBreakpoints() -> Breakpoints* { return &m_breakpoints; };
		auto GetWatchpoints() -> Watchpoints* { return &m_watchpoints; };

		void LoadDebugData(const std::string& _path);
		void SaveDebugData();

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

		std::string m_debugPath;

		UpdateId m_labelsUpdates = 0;
		UpdateId m_constsUpdates = 0;
		UpdateId m_commentsUpdates = 0;
		UpdateId m_editsUpdates = 0;
		UpdateId m_codePerfsUpdates = 0;
	};
}