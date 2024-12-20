#pragma once

#include <mutex>
#include <string>

#include "utils/consts.h"
#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "core/hardware.h"
#include "core/debugger.h"

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
		static constexpr ImU32 DIS_CLR_LINK_HIGHLIGHT = dev::IM_U32(0xD010FFFF);

		struct ContextMenu {
			enum class Status{ NONE = 0, INIT_CONTEXT_MENU, INIT_COMMENT_EDIT, INIT_LABEL_EDIT, INIT_CONST_EDIT };
			Status status = Status::NONE;
			Addr addr = 0;
			std::string str;
			bool immHovered = false; // the context menu was opened on the immediate operand
			const char* contextMenuName = "DisasmItemMenu";
			const char* commentEditName = "Edit Comment";
			const char* labelEditName = "Edit Label";
			const char* constEditName = "Edit Const";

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
		ImFont* m_fontCommentP = nullptr;
		ReqUI& m_reqUI;
		char m_searchText[255] = "";
		Addr m_disasmAddr = 0;
		const Disasm::Lines** m_disasmPP = nullptr;
		int m_disasmLines = Disasm::DISASM_LINES_MAX;
		const Disasm::ImmAddrLinks* m_immLinksP = nullptr;
		size_t m_immLinksNum = 0;
		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;
		int m_selectedLineIdx = 0;
		static constexpr int NAVIGATE_ADDRS_LEN = 256;
		std::array<Addr, NAVIGATE_ADDRS_LEN> m_navigateAddrs; // contains addr where the user navigated to
		int m_navigateAddrsIdx = 0; // points to the current addr in m_navigateAddrs
		int m_navigateAddrsSize = 0; // the length of the stored addrs. it's how many times the user navigated through links without moving back.

		void DrawDebugControls(const bool _isRunning);
		void DrawSearch(const bool _isRunning);
		void DrawDisasm(const bool _isRunning);
		void DrawDisasmIcons(const bool _isRunning, const Disasm::Line& _line, const int _lineIdx, const Addr _regPC);
		void DrawDisasmAddr(const bool _isRunning, const Disasm::Line& _line, 
			ReqUI& _reqUI, ContextMenu& _contextMenu, AddrHighlight& _addrHighlight);
		void DrawDisasmCode(const bool _isRunning, const Disasm::Line& _line,
			ReqUI& _reqUI, ContextMenu& _contextMenu, AddrHighlight& _addrHighlight);
		void DrawDisasmComment(const Disasm::Line& _line);
		void DrawDisasmLabels(const Disasm::Line& _line);
		void DrawDisasmStats(const Disasm::Line& _line);
		void DrawContextMenu(const Addr _regPC, ContextMenu& _contextMenu);
		void DrawCommentEdit(ContextMenu& _contextMenu);
		void DrawLabelEdit(ContextMenu& _contextMenu);
		void DrawConstEdit(ContextMenu& _contextMenu);
		void DrawAddrLinks(const bool _isRunning, const int _lineIdx,
			const bool _selected);
		void UpdateData(const bool _isRunning);
		void ReqHandling();
		bool IsDisasmTableOutOfWindow() const;
		auto GetVisibleLines() const -> int;

	public:

		DisasmWindow(Hardware& _hardware, Debugger& _debugger, ImFont* fontComment, 
			const float* const _dpiScaleP, 
			ReqUI& _reqUI);
		void Update(bool& _visible, const bool _isRunning);
		void UpdateDisasm(const Addr _addr, const int _instructionsOffset = DISASM_INSTRUCTION_OFFSET,
			const bool _updateSelection = true);
	};

};