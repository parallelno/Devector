#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "core/hardware.h"

namespace dev
{
	class SymbolsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Hardware& m_hardware;
		Debugger& m_debugger;
		ReqUI& m_reqUI;

		int64_t m_ccLast = -1; // to force the first stats update

		DebugData::UpdateId m_labelsUpdates = 0;
		DebugData::UpdateId m_constsUpdates = 0;
		DebugData::UpdateId m_commentsUpdates = 0;

		DebugData::SymbolAddrList m_filteredLabels;
		DebugData::SymbolAddrList m_filteredConsts;
		DebugData::SymbolAddrList m_filteredComments;

		std::string m_labelFilter;
		std::string m_constFilter;
		std::string m_commentFilter;

		std::string m_tempFilter;

		int m_selectedLineIdx = 0;
		
		enum class SymbolType { LABEL = 0, CONST, COMMENT };

		struct ContextMenu {
			enum class Status { NONE = 0, INIT_CONTEXT_MENU, CONTEXT_MENU, INIT_SYMBOL_EDIT, SYMBOL_EDIT, INIT_ADDR_EDIT, ADDR_EDIT, INIT_ADD_SYMBOL, ADD_SYMBOL };
			Status status = Status::NONE;
			SymbolType symbolType = SymbolType::LABEL;
			Addr addr = 0;
			std::string symbol = "";
			bool immHovered = false; // the context menu was opened on the immediate operand
			const char* contextMenuName = "DisasmItemMenu";

			void Init(Addr _addr, const std::string& _symbol, const SymbolType _symbolType)
			{
				status = Status::INIT_CONTEXT_MENU;
				symbolType = _symbolType;
				addr = _addr;
				symbol = _symbol;
			}
		};
		ContextMenu m_contextMenu;

		void UpdateData(const bool _isRunning);

		void UpdateAndDrawFilteredSymbols(DebugData::SymbolAddrList& _filteredSymbols,
										DebugData::UpdateId& _filteredUpdateId, 
										const DebugData::UpdateId& _updateId,
										std::string& _filter,
										SymbolType _symbolType);

		void DrawContextMenu(ContextMenu& _contextMenu);
		void DrawContextMenuMain(ContextMenu& _contextMenu);
		void DrawContextMenuSymbolEdit(ContextMenu& _contextMenu, std::string& _newName);
		void DrawContextMenuAddrEdit(ContextMenu& _contextMenu, int& _newAddr);

	public:
		SymbolsWindow(Hardware& _hardware, Debugger& _debugger, 
			const float* const _dpiScaleP,
			ReqUI& _reqUI);
		void Update(bool& _visible, const bool _isRunning);
		void Draw(const bool _isRunning);
	};
};