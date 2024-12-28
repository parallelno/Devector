#include "ui/symbols_window.h"

#include <format>
#include "utils/str_utils.h"
#include "imgui_stdlib.h"

dev::SymbolsWindow::SymbolsWindow(Hardware& _hardware, Debugger& _debugger,
	const float* const _dpiScaleP,
	ReqUI& _reqUI)
	:
	BaseWindow("Symbols & Comments", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _dpiScaleP),
	m_hardware(_hardware), m_debugger(_debugger), 
	m_reqUI(_reqUI)
{}

void dev::SymbolsWindow::Update(bool& _visible, const bool _isRunning)
{
	BaseWindow::Update();

	if (_visible && ImGui::Begin(m_name.c_str(), &_visible, ImGuiWindowFlags_NoCollapse))
	{
		UpdateData(_isRunning);
		Draw(_isRunning);

		ImGui::End();
	}
}

void dev::SymbolsWindow::Draw(const bool _isRunning)
{
	// three tabs: consts, labels, comments
	if (ImGui::BeginTabBar("SymbolsTabs"))
	{
		if (ImGui::BeginTabItem("Labels"))
		{
			auto updateId = m_debugger.GetDebugData().GetLabelsUpdates();
			UpdateAndDrawFilteredSymbols(m_filteredLabels, m_labelsUpdates, updateId,
										m_labelFilter, SymbolType::LABEL);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Consts"))
		{
			auto updateId = m_debugger.GetDebugData().GetConstsUpdates();
			UpdateAndDrawFilteredSymbols(m_filteredConsts, m_constsUpdates, updateId,
										m_constFilter, SymbolType::CONST);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Comments"))
		{
			auto updateId = m_debugger.GetDebugData().GetCommentsUpdates();
			UpdateAndDrawFilteredSymbols(m_filteredComments, m_commentsUpdates, updateId,
										m_commentFilter, SymbolType::COMMENT);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

void dev::SymbolsWindow::UpdateData(const bool _isRunning)
{
	// check if the hardware updated its state
	uint64_t cc = m_hardware.Request(Hardware::Req::GET_CC)->at("cc");
	auto ccDiff = cc - m_ccLast;
	if (ccDiff == 0) return;
	m_ccLast = cc;

	// update the data
}

void dev::SymbolsWindow::UpdateAndDrawFilteredSymbols(
	DebugData::SymbolAddrList& _filteredSymbols, 
	DebugData::UpdateId& _filteredUpdateId, const DebugData::UpdateId& _updateId,
	std::string& _filter, SymbolType _symbolType)
{
	// update the data and draw a filter
	ImGui::Text("Filter"); ImGui::SameLine();
	bool filterUpdated = ImGui::InputTextWithHint("##filter", "search name", &_filter, ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::SameLine(); dev::DrawHelpMarker(
		"Accepts substrings. Case insensitive.\n\
Double click on the symbol to locate the addr in the Disasm Window.\n\
Double click + Ctrl the symbol to locate the addr in the Hex Window.");

	if (filterUpdated || _filteredUpdateId != _updateId)
	{
		// update filtered labels
		_filteredUpdateId = _updateId;

		switch (_symbolType)
		{
		case SymbolType::LABEL:
			m_debugger.GetDebugData().GetFilteredLabels(_filteredSymbols, _filter);
			break;

		case SymbolType::CONST:
			m_debugger.GetDebugData().GetFilteredConsts(_filteredSymbols, _filter);
			break;

		case SymbolType::COMMENT:
			m_debugger.GetDebugData().GetFilteredComments(_filteredSymbols, _filter);
			break;
		
		default:
			break;
		}
	}

	int hoveredLineIdx = -1;
	bool rmbPressed = false;

	// draw a table
	const int COLUMNS_COUNT = 3;
	const char* tableName = "Symbols";

	static ImGuiTableFlags flags =
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_NoBordersInBodyUntilResize |
		ImGuiTableFlags_Resizable;
	if (ImGui::BeginTable(tableName, COLUMNS_COUNT, flags))
	{
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0); // for the line selection/highlight
		ImGui::TableSetupColumn("consts", ImGuiTableColumnFlags_WidthFixed, 100);
		ImGui::TableSetupColumn("Addr");

		ImGuiListClipper clipper;
		clipper.Begin(int(_filteredSymbols.size()));
		while (clipper.Step())
		{
			for (int lineIdx = clipper.DisplayStart; lineIdx < clipper.DisplayEnd; lineIdx++)
			{
				ImGui::TableNextRow();

				// the line selection/highlight
				ImGui::TableNextColumn();
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DIS_BG_CLR_BRK);
				const bool isSelected = m_selectedLineIdx == lineIdx;
				if (ImGui::Selectable(std::format("##SymbolLineId{}", lineIdx).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
				{
					m_selectedLineIdx = lineIdx;
				}
				
				hoveredLineIdx = ImGui::IsItemHovered() ? lineIdx : hoveredLineIdx;
				rmbPressed |= ImGui::IsMouseClicked(ImGuiMouseButton_Right);

				const auto& [label, addr, addrS] = _filteredSymbols.at(lineIdx);

				// draw the symbol
				ImGui::TableNextColumn();
				ImGui::TextColored( label[0] == '@' ? DASM_CLR_LABEL_LOCAL : DASM_CLR_LABEL_GLOBAL, label.c_str());

				// draw the addr
				ImGui::TableNextColumn();
				ImGui::TextColored( DASM_CLR_ADDR, addrS.c_str());

				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					const auto& [symbol, selectedAddr, selectedAddrS] = _filteredSymbols.at(m_selectedLineIdx);
					m_reqUI.globalAddr = selectedAddr;

					if (ImGui::GetIO().KeyCtrl)
					{
						m_reqUI.type = ReqUI::Type::HEX_HIGHLIGHT_ON;
						m_reqUI.len = 1;
					}
					else{
						m_reqUI.type = ReqUI::Type::DISASM_NAVIGATE_TO_ADDR;
					}
				}
				
				if (hoveredLineIdx >= 0 && rmbPressed )
				{
					const auto& [symbol, selectedAddr, selectedAddrS] = _filteredSymbols.at(hoveredLineIdx);
					m_contextMenu.Init(selectedAddr, symbol, _symbolType);
				}
				
			}
		}
		ImGui::EndTable();
	}

	DrawContextMenu(m_contextMenu);
}

void dev::SymbolsWindow::DrawContextMenu(ContextMenu& _contextMenu)
{
	static int newAddr = 0;
	static std::string newSymbol;

	switch (_contextMenu.status)
	{
	case ContextMenu::Status::INIT_CONTEXT_MENU:
		ImGui::OpenPopup(_contextMenu.contextMenuName);
		_contextMenu.status = ContextMenu::Status::CONTEXT_MENU;
		break;

	case ContextMenu::Status::INIT_SYMBOL_EDIT:
		ImGui::OpenPopup(_contextMenu.contextMenuName);
		_contextMenu.status = ContextMenu::Status::SYMBOL_EDIT;
		newSymbol = _contextMenu.symbol;
		break;
	
	case ContextMenu::Status::INIT_ADDR_EDIT:
		ImGui::OpenPopup(_contextMenu.contextMenuName);
		_contextMenu.status = ContextMenu::Status::ADDR_EDIT;
		newAddr = _contextMenu.addr;
		break;

	case ContextMenu::Status::INIT_ADD_SYMBOL:
		ImGui::OpenPopup(_contextMenu.contextMenuName);
		_contextMenu.status = ContextMenu::Status::ADD_SYMBOL;
		break;

	default:
		break;
	}

	if (ImGui::BeginPopup(_contextMenu.contextMenuName))
	{
		switch (_contextMenu.status)
		{
		case ContextMenu::Status::CONTEXT_MENU:
			DrawContextMenuMain(_contextMenu);
			break;
		
		case ContextMenu::Status::SYMBOL_EDIT:
			DrawContextMenuSymbolEdit(_contextMenu, newSymbol);
			break;
		
		case ContextMenu::Status::ADDR_EDIT:
			DrawContextMenuAddrEdit(_contextMenu, newAddr);
			break;

		case ContextMenu::Status::ADD_SYMBOL:
			DrawContextMenuSymbolAdd(_contextMenu, newAddr, newSymbol);
			break;
		}

		ImGui::EndPopup();
	}
}


void dev::SymbolsWindow::DrawContextMenuMain(ContextMenu& _contextMenu)
{
	if (ImGui::MenuItem("Copy Symbol Name")) {
		dev::CopyToClipboard(_contextMenu.symbol);
		_contextMenu.status = ContextMenu::Status::NONE;
		ImGui::CloseCurrentPopup();
	}

	if (ImGui::MenuItem("Copy Symbol Addr")) {
		dev::CopyToClipboard(std::format("0x{:X}", (_contextMenu.addr)));
		_contextMenu.status = ContextMenu::Status::NONE;
		ImGui::CloseCurrentPopup();
	}

	ImGui::SeparatorText("");

	if (ImGui::MenuItem("Locate in the Disasm WIndow")) {
		m_reqUI.type = ReqUI::Type::DISASM_NAVIGATE_TO_ADDR;
		m_reqUI.globalAddr = _contextMenu.addr;
		_contextMenu.status = ContextMenu::Status::NONE;
		ImGui::CloseCurrentPopup();
	}

	if (ImGui::MenuItem("Locate in the Hex WIndow")) {
		m_reqUI.type = ReqUI::Type::HEX_HIGHLIGHT_ON;
		m_reqUI.globalAddr = _contextMenu.addr;
		m_reqUI.len = 1;
		_contextMenu.status = ContextMenu::Status::NONE;
		ImGui::CloseCurrentPopup();
	}

	ImGui::SeparatorText("");

	if (ImGui::MenuItem("Add Symbol"))
	{
		_contextMenu.status = ContextMenu::Status::INIT_ADD_SYMBOL;
		ImGui::CloseCurrentPopup();
	}

	if (ImGui::MenuItem("Delete Symbol"))
	{
		switch (_contextMenu.symbolType)
		{
		case SymbolType::LABEL:
			m_debugger.GetDebugData().DelLabel(_contextMenu.addr, _contextMenu.symbol);
			break;

		case SymbolType::CONST:
			m_debugger.GetDebugData().DelConst(_contextMenu.addr, _contextMenu.symbol);
			break;

		case SymbolType::COMMENT:
			m_debugger.GetDebugData().DelComment(_contextMenu.addr);
			break;
		
		default:
			break;
		}
		m_reqUI.type = dev::ReqUI::Type::DISASM_UPDATE;
		_contextMenu.status = ContextMenu::Status::NONE;
		ImGui::CloseCurrentPopup();
	}
	
	ImGui::SeparatorText("");

	if (ImGui::MenuItem("Edit Symbol Name")){
		_contextMenu.status = ContextMenu::Status::INIT_SYMBOL_EDIT;
		ImGui::CloseCurrentPopup();
	}

	if (ImGui::MenuItem("Edit Symbol Addr")) {
		_contextMenu.status = ContextMenu::Status::INIT_ADDR_EDIT;
		ImGui::CloseCurrentPopup();
	}
}

void dev::SymbolsWindow::DrawContextMenuSymbolEdit(ContextMenu& _contextMenu, std::string& _newName)
{
	int caseFlag = _contextMenu.symbolType == SymbolType::CONST ? ImGuiInputTextFlags_CharsUppercase : 0;
	bool pressedEnter = ImGui::InputTextWithHint("##SymbolEdit", "", &_newName, ImGuiInputTextFlags_EnterReturnsTrue | caseFlag);
	ImGui::SameLine();
	bool pressedOk = ImGui::Button("OK");
	ImGui::SameLine();

	if (ImGui::Button("Cancel"))
	{
		ImGui::CloseCurrentPopup();
		_contextMenu.status = ContextMenu::Status::NONE;
	}

	if (pressedEnter || pressedOk)
	{
		switch (_contextMenu.symbolType)
		{
		case SymbolType::LABEL:
			m_debugger.GetDebugData().RenameLabel(_contextMenu.addr, _contextMenu.symbol, _newName);
			break;

		case SymbolType::CONST:
			m_debugger.GetDebugData().RenameConst(_contextMenu.addr, _contextMenu.symbol, _newName);
			break;

		case SymbolType::COMMENT:
			m_debugger.GetDebugData().SetComment(_contextMenu.addr, _newName);
			break;
		
		default:
			break;
		}

		ImGui::CloseCurrentPopup();
		_contextMenu.status = ContextMenu::Status::NONE;
		m_reqUI.type = dev::ReqUI::Type::DISASM_UPDATE;
	}
}

void dev::SymbolsWindow::DrawContextMenuAddrEdit(ContextMenu& _contextMenu, int& _newAddr)
{
	ImGui::InputInt("##AddrEdit", &_newAddr, 1, 100, ImGuiInputTextFlags_CharsHexadecimal);
	bool pressedEnter = ImGui::IsKeyPressed(ImGuiKey_Enter);
	ImGui::SameLine();
	bool pressedOk = ImGui::Button("OK");
	ImGui::SameLine();

	if (ImGui::Button("Cancel"))
	{
		ImGui::CloseCurrentPopup();
		_contextMenu.status = ContextMenu::Status::NONE;
	}

	if (pressedEnter || pressedOk)
	{
		switch (_contextMenu.symbolType)
		{
		case SymbolType::LABEL:
			m_debugger.GetDebugData().DelLabel(_contextMenu.addr, _contextMenu.symbol);
			m_debugger.GetDebugData().AddLabel(_newAddr, _contextMenu.symbol);
			break;

		case SymbolType::CONST:
			m_debugger.GetDebugData().DelConst(_contextMenu.addr, _contextMenu.symbol);
			m_debugger.GetDebugData().AddConst(_newAddr, _contextMenu.symbol);
			break;

		case SymbolType::COMMENT:
			m_debugger.GetDebugData().DelComment(_contextMenu.addr);
			m_debugger.GetDebugData().SetComment(_newAddr, _contextMenu.symbol);
			break;
		
		default:
			break;
		}

		ImGui::CloseCurrentPopup();
		m_reqUI.type = dev::ReqUI::Type::DISASM_UPDATE;
		_contextMenu.status = ContextMenu::Status::NONE;
	}	
}

void dev::SymbolsWindow::DrawContextMenuSymbolAdd(ContextMenu& _contextMenu, int& _newAddr , std::string& _newName)
{
	int caseFlag = _contextMenu.symbolType == SymbolType::CONST ? ImGuiInputTextFlags_CharsUppercase : 0;
	bool pressedEnter = ImGui::InputTextWithHint("##SymbolAdd", "", &_newName, ImGuiInputTextFlags_EnterReturnsTrue | caseFlag);
	ImGui::SameLine();
	ImGui::InputInt("##AddrEdit", &_newAddr, 1, 100, ImGuiInputTextFlags_CharsHexadecimal);
	pressedEnter |= ImGui::IsKeyPressed(ImGuiKey_Enter);
	ImGui::SameLine();
	bool pressedOk = ImGui::Button("OK");
	ImGui::SameLine();

	if (ImGui::Button("Cancel"))
	{
		ImGui::CloseCurrentPopup();
		_contextMenu.status = ContextMenu::Status::NONE;
	}

	if (pressedEnter || pressedOk)
	{
		switch (_contextMenu.symbolType)
		{
		case SymbolType::LABEL:
			m_debugger.GetDebugData().AddLabel(_newAddr, _newName);
			break;

		case SymbolType::CONST:
			m_debugger.GetDebugData().AddConst(_newAddr, _newName);
			break;

		case SymbolType::COMMENT:
			m_debugger.GetDebugData().SetComment(_newAddr, _newName);
			break;
		
		default:
			break;
		}

		ImGui::CloseCurrentPopup();
		_contextMenu.status = ContextMenu::Status::NONE;
		m_reqUI.type = dev::ReqUI::Type::DISASM_UPDATE;
	}
}
