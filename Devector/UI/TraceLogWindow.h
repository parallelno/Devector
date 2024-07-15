#pragma once

#include <vector>

#include "Utils/Types.h"
#include "Utils/Consts.h"
#include "Ui/BaseWindow.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/Result.h"
#include "Core/Hardware.h"
#include "Core/Debugger.h"

namespace dev
{
	class TraceLogWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 512;
		static constexpr int DEFAULT_WINDOW_H = 300;
		static constexpr float ADDR_W = 50.0f;
		static constexpr float CODE_W = 200.0f;
		
		static constexpr int MAX_DISASM_LABELS = 4;

		struct ContextMenu {
			enum class Status { NONE = 0, INIT_CONTEXT_MENU, INIT_COMMENT_EDIT, INIT_LABEL_EDIT, INIT_CONST_EDIT };
			Status status = Status::NONE;
			Addr addr = 0;
			std::string str;
			bool immHovered = false; // the context menu was opened on the immediate operand
			const char* contextMenuName = "DisasmItemMenu";

			void Init(Addr _addr, const std::string& _lineS, const bool _immHovered = false)
			{
				immHovered = _immHovered;
				status = Status::INIT_CONTEXT_MENU;
				addr = _addr;
				str = _lineS;
			}
		};
		ContextMenu m_contextMenu;

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
		ReqDisasm& m_reqDisasm;
		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;
		const TraceLog::Lines* m_traceLogP = nullptr;
		uint8_t m_disasmFilter = 0;
		int m_selectedLineIdx = 0;
		size_t m_disasmLinesLen = 0;

		void UpdateData(const bool _isRunning);
		void DrawLog(const bool _isRunning);
		void DrawContextMenu(const Addr _regPC, ContextMenu& _contextMenu);
		void DrawDisasmCode(const bool _isRunning, const Disasm::Line& _line,
			ReqDisasm& _reqDisasm, ContextMenu& _contextMenu, AddrHighlight& _addrHighlight);
		void DrawDisasmAddr(const bool _isRunning, const Disasm::Line& _line,
			ReqDisasm& _reqDisasm, ContextMenu& _contextMenu, AddrHighlight& _addrHighlight);

	public:
		TraceLogWindow(Hardware& _hardware, Debugger& _debugger, const float* const _fontSizeP, 
				const float* const _dpiScaleP, ReqDisasm& _reqDisasm);
		void Update(bool& _visible);
	};
};