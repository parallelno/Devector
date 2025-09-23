#pragma once

#include <mutex>
#include <string>

#include "utils/consts.h"
#include "utils/imgui_utils.h"
#include "utils/history.h"
#include "ui/base_window.h"
#include "core/hardware.h"
#include "core/debugger.h"
#include "core/disasm.h"
#include "scheduler.h"

namespace dev
{
	static constexpr int DEFAULT_WINDOW_W = 600;
	static constexpr int DEFAULT_WINDOW_H = 800;

	class DisasmWindow : public BaseWindow
	{
		// Set column widths
		static constexpr float BRK_W = 40;
		static constexpr float ADDR_W = 50.0f;
		static constexpr float CODE_W = 200.0f;
		static constexpr float COMMAND_W = 120.0f;
		static constexpr float STATS_W = 100.0f;
		static constexpr float IMM_ADDR_LINK_POS_X = BRK_W + 10.0f;
		static constexpr float IMM_ADDR_LINK_AREA_W = 30;

		static constexpr float PC_ICON_OFFSET_X = 30.0f;

		static constexpr int DISASM_INSTRUCTION_OFFSET = 6;

		static constexpr int MAX_DISASM_LABELS = 20;

		static constexpr ImU32 DIS_CLR_LINK		= dev::IM_U32(0x808010FF);
		static constexpr ImU32 DIS_CLR_LINK_MINOR = dev::IM_U32(0xD0C443FF);
		static constexpr ImU32 DIS_CLR_LINK_HIGHLIGHT = dev::IM_U32(0xD005F0FF);


		struct AddrHighlight
		{
			int addr = -1; // -1 means disabled

			void Init(const Addr _addr) {
				addr = _addr;
			}
			bool IsEnabled(const Addr _addr)
			{
				bool out = _addr == addr;
				if (_addr == addr) addr = -1; // disable after use
				return out;
			}
		};
		AddrHighlight m_addrHighlight;

		Hardware& m_hardware;
		Debugger& m_debugger;
		ImFont* m_fontCommentP = nullptr;
		char m_searchText[255] = "";
		Addr m_disasmAddr = 0;
		int m_disasmLines = 0;
		const Disasm::ImmAddrLinks* m_immLinksP = nullptr;
		int m_selectedLineAddr = 0;

		static constexpr int NAVIGATE_HISTORY_MAX = 5;
		dev::History<Addr> m_navigateHistory =
								dev::History<Addr>(NAVIGATE_HISTORY_MAX);

		void CallbackUpdateAtCC(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data);
		void CallbackUpdateAtAddr(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);
		void UpdateDisasm(
			const Addr _addr,
			const int _instructionsOffset = DISASM_INSTRUCTION_OFFSET,
			const bool _updateSelection = true);

		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;
		void DrawDebugControls(const bool _isRunning);
		void DrawSearch(const bool _isRunning);
		void DrawDisasm(const bool _isRunning);
		void DrawDisasmIcons(
			const bool _isRunning, const DisasmLine& _line,
			const int _lineIdx, const Addr _regPC);
		void DrawDisasmAddr(const bool _isRunning, const DisasmLine& _line,
			AddrHighlight& _addrHighlight);
		void DrawDisasmCode(const bool _isRunning, const DisasmLine& _line,
			AddrHighlight& _addrHighlight);
		void DrawDisasmComment(const DisasmLine& _line);
		void DrawDisasmLabels(const DisasmLine& _line);
		void DrawDisasmStats(const DisasmLine& _line);
		void DrawAddrLinks(const bool _isRunning,
			const int _lineIdx,
			const bool _selected);
		void DrawAddrLink(ImVec2 _pos0, ImVec2 _pos1, int _lineIdx,
			ImU32 _linkColor, int _endLineIdx, float _linkWidth);
		void DrawNextExecutedLineHighlight(
			const bool _isRunning,
			const DisasmLine& _line,
			const Addr _regPC);

		auto GetVisibleLines() const -> int;
		void CheckControls(const Addr _addr);

	public:

		DisasmWindow(Hardware& _hardware, Debugger& _debugger,
			ImFont* fontComment,
			dev::Scheduler& _scheduler,
			bool* _visibleP);
	};

};