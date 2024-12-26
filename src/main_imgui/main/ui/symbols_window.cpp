#include "ui/symbols_window.h"

#include <format>
#include "utils/str_utils.h"
#include "imgui_stdlib.h"

dev::SymbolsWindow::SymbolsWindow(Hardware& _hardware, Debugger& _debugger,
	const float* const _dpiScaleP,
	ReqUI& _reqUI)
	:
	BaseWindow("Symbols", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _dpiScaleP),
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
			UpdateAndDrawFilteredSymbols(m_filteredLabels, m_labelsUpdates, 
										m_labelFilter, &DebugData::GetFilteredLabels);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Consts"))
		{
			UpdateAndDrawFilteredSymbols(m_filteredConsts, m_constsUpdates, 
										m_constFilter, &DebugData::GetFilteredConsts);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Comments"))
		{
			UpdateAndDrawFilteredSymbols(m_filteredComments, m_commentsUpdates, 
										m_commentFilter, &DebugData::GetFilteredComments);
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

void dev::SymbolsWindow::UpdateAndDrawFilteredSymbols(DebugData::SymbolAddrList& _filteredSymbols, DebugData::UpdateId& _updateId, std::string& _filter,
													void (DebugData::*getFilteredFunc)(DebugData::SymbolAddrList& _out, const std::string& _filter) const)
{
	// update the data and draw a filter
	ImGui::Text("Filter"); ImGui::SameLine();
	bool filterUpdated = ImGui::InputTextWithHint("##filter", "symbol_name", &_filter, ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::SameLine(); dev::DrawHelpMarker("Accepts substrings. Case insensitive. \nDouble click to navigate the addr in the Disasm Window. \nIf pressed Ctrl, the Hex Window highlights the addr.");

	auto labelUpdates = m_debugger.GetDebugData().GetLabelsUpdates();
	if (filterUpdated || _updateId != labelUpdates)
	{
		// update filtered labels
		_updateId = labelUpdates;
		(m_debugger.GetDebugData().*getFilteredFunc)(_filteredSymbols, _filter);
	}


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

				const auto& [label, addr, addrS] = _filteredSymbols.at(lineIdx);

				ImGui::TableNextColumn();
				ImGui::TextColored( label[0] == '@' ? DASM_CLR_LABEL_LOCAL : DASM_CLR_LABEL_GLOBAL, label.c_str());

				ImGui::TableNextColumn();
				ImGui::TextColored( DASM_CLR_ADDR, addrS.c_str());


				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					const auto& [selectedLabel, selectedAddr, selectedAddrS] = _filteredSymbols.at(m_selectedLineIdx);
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
				
			}
		}
		ImGui::EndTable();
	}
}