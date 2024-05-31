#pragma once

#include <mutex>
#include <string>

#include "Utils/Consts.h"
#include "Utils/ImGuiUtils.h"
#include "UI/BaseWindow.h"
#include "Core/Hardware.h"
#include "Core/Debugger.h"

namespace dev
{
	static constexpr int DEFAULT_WINDOW_W = 600;
	static constexpr int DEFAULT_WINDOW_H = 800;

	class DisasmWindow : public BaseWindow
	{	
		// Set column widths
		static constexpr float BRK_W = 20;
		static constexpr float ADDR_W = 50.0f;
		static constexpr float CODE_W = 200.0f;
		static constexpr float COMMAND_W = 120.0f;
		static constexpr float STATS_W = 100.0f;
		static constexpr int DISASM_INSTRUCTION_OFFSET = 6;
		const char* m_itemContextMenu = "DisasmItemMenu";
		static constexpr int ADDR_HIGHLIGHT_TIME = 1000;
		static constexpr int MAX_DISASM_LABELS = 20;
		
		Hardware& m_hardware;
		Debugger& m_debugger;
		ImFont* m_fontCommentP = nullptr;
		ReqDisasm& m_reqDisasm;
		char m_searchText[255] = "";
		Debugger::DisasmLines m_disasm;
		const Disasm::Lines* m_disasmP = nullptr;
		int m_disasmLines = Disasm::DISASM_LINES_MAX;
		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;
		int m_selectedLineIdx = 0;
		bool& m_reqHardwareStatsReset;
		bool& m_reqMainWindowReload;
		static constexpr int NAVIGATE_ADDRS_LEN = 256;
		std::array<Addr, NAVIGATE_ADDRS_LEN> m_navigateAddrs;
		int m_navigateAddrsIdx = 0;
		int m_navigateAddrsSize = 0;

		void DrawDebugControls(const bool _isRunning);
		void DrawSearch(const bool _isRunning);
		void DrawDisasm(const bool _isRunning);
		void DrawDsasmIcons(const bool _isRunning, const Disasm::Line& _line, const int _lineIdx, const Addr _regPC);
		auto DrawDisasmAddr(const bool _isRunning, const Disasm::Line& _line, const uint8_t _addrHighlightedAlpha,
			int& _copyToClipboardAddr, bool& _openItemContextMenu) -> ImVec2;
		void DrawDisasmCode(const bool _isRunning, const Disasm::Line& _line,
			int& _addrHighlighted, int& _addrHighlightedTimer, int& _copyToClipboardAddr, bool& _openItemContextMenu);
		auto DrawDisasmComment(const Disasm::Line& _line) -> ImVec2;
		auto DrawDisasmLabels(const Disasm::Line& _line) -> ImVec2;
		void DrawDisasmStats(const Disasm::Line& _line);
		void DrawDisasmConsts(const Disasm::Line& _line);
		void UpdateData(const bool _isRunning);
		bool IsDisasmTableOutOfWindow() const;
		int GetVisibleLines() const;
		int DrawDisasmContextMenu(const bool _openContextMenu, const Addr _regPC, int _addr, 
			int _copyToClipboardAddr, std::string& _str);

	public:

		DisasmWindow(Hardware& _hardware, Debugger& _debugger, ImFont* fontComment, 
			const float* const _fontSizeP, const float* const _dpiScaleP, 
			ReqDisasm& _reqDisasm, bool& _reset, bool& _reload);
		void Update();
		void UpdateDisasm(const Addr _addr, const int _instructionsOffset = DISASM_INSTRUCTION_OFFSET,
			const bool _updateSelection = true);
	};

};