#include "ui/debugdata_window.h"

#include <format>
#include "utils/str_utils.h"
#include "imgui_stdlib.h"

dev::DebugDataWindow::DebugDataWindow(
	Hardware& _hardware, Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool& _visible, const float* const _dpiScaleP)
	:
	BaseWindow("Debug Data", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visible, _dpiScaleP),
	m_hardware(_hardware), m_debugger(_debugger)
{
	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::HW_RUNNING | dev::Signals::BREAK,
			std::bind(&dev::DebugDataWindow::CallbackUpdateData,
				this, std::placeholders::_1, std::placeholders::_2),
			m_visible, 1000ms));
}

void dev::DebugDataWindow::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	if (ImGui::BeginTabBar("ElementsTabs"))
	{
		if (ImGui::BeginTabItem("Labels"))
		{
			auto updateId = m_debugger.GetDebugData().GetLabelsUpdates();
			UpdateAndDrawFilteredElements(
				m_filteredLabels, m_labelsUpdates, updateId,
				m_labelFilter, ElementType::LABEL);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Consts"))
		{
			auto updateId = m_debugger.GetDebugData().GetConstsUpdates();
			UpdateAndDrawFilteredElements(
				m_filteredConsts, m_constsUpdates, updateId,
				m_constFilter, ElementType::CONST);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Comments"))
		{
			auto updateId = m_debugger.GetDebugData().GetCommentsUpdates();
			UpdateAndDrawFilteredElements(
				m_filteredComments, m_commentsUpdates, updateId,
				m_commentFilter, ElementType::COMMENT);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Memory Edits"))
		{
			auto updateId = m_debugger.GetDebugData().GetEditsUpdates();
			UpdateAndDrawFilteredElements(
				m_filteredEdits, m_editsUpdates, updateId,
				m_editFilter, ElementType::MEMORY_EDIT);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Code Perfs"))
		{
			auto updateId = m_debugger.GetDebugData().GetCodePerfsUpdates();
			UpdateAndDrawFilteredElements(
				m_filteredCodePerfs, m_codePerfsUpdates, updateId,
				m_codePerfFilter, ElementType::CODE_PERFS);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Scripts"))
		{
			auto updateId = m_debugger.GetDebugData().GetScripts().GetUpdates();
			UpdateAndDrawFilteredElements(
				m_filteredScripts, m_scriptsUpdates, updateId,
				m_scriptFilter, ElementType::SCRIPTS);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

void dev::DebugDataWindow::CallbackUpdateData(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	// TODO: move the data update here
}

void dev::DebugDataWindow::UpdateAndDrawFilteredElements(
	DebugData::FilteredElements& _filteredElements,
	DebugData::UpdateId& _filteredUpdateId, const DebugData::UpdateId& _updateId,
	std::string& _filter, ElementType _elementType)
{
	// update the data and draw a filter
	ImGui::Text("Filter"); ImGui::SameLine();
	bool filterUpdated = ImGui::InputTextWithHint("##filter", "search name",
		&_filter);
	ImGui::SameLine(); dev::DrawHelpMarker(
		"Accepts substrings. Case insensitive.\n"
		"Double click on the element to locate the addr in the Disasm Window.\n"
		"Double click + Ctrl the element to locate the addr in the Hex Window.");

	if (filterUpdated || _filteredUpdateId != _updateId)
	{
		// update filtered labels
		_filteredUpdateId = _updateId;

		switch (_elementType)
		{
		case ElementType::LABEL:
			m_debugger.GetDebugData().GetFilteredLabels(_filteredElements, _filter);
			break;

		case ElementType::CONST:
			m_debugger.GetDebugData().GetFilteredConsts(_filteredElements, _filter);
			break;

		case ElementType::COMMENT:
			m_debugger.GetDebugData().GetFilteredComments(_filteredElements, _filter);
			break;

		case ElementType::MEMORY_EDIT:
			m_debugger.GetDebugData().GetFilteredMemoryEdits(_filteredElements, _filter);
			break;
		case ElementType::SCRIPTS:
			m_debugger.GetDebugData().GetFilteredScripts(_filteredElements, _filter);
			break;
		default:
			break;
		}
	}

	// update the CODE_PERFS tab every second
	static auto lastTime = std::chrono::steady_clock::now();
	auto now = std::chrono::steady_clock::now();
	auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime).count();
	bool timeout = diff > 0;
	if (_elementType == ElementType::CODE_PERFS && timeout ){
		lastTime = now;
		m_debugger.GetDebugData().GetFilteredCodePerfs(_filteredElements, _filter);
	}

	// draw a table
	const int COLUMNS_COUNT = 3;
	const char* tableName = "Elements";
	ImVec2 itemsMin = ImGui::GetCursorStartPos();
	ImVec2 itemsMax;
	int hoveredLineIdx = -1;

	static ImGuiTableFlags flags =
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_NoBordersInBodyUntilResize |
		ImGuiTableFlags_Resizable;
	if (ImGui::BeginTable(tableName, COLUMNS_COUNT, flags))
	{
		ImGui::TableSetupColumn("",
			ImGuiTableColumnFlags_WidthFixed |
			ImGuiTableColumnFlags_NoResize, 0); // for the line selection/highlight
		ImGui::TableSetupColumn("consts", ImGuiTableColumnFlags_WidthFixed, 100);
		ImGui::TableSetupColumn("Addr");

		ImGuiListClipper clipper;
		clipper.Begin(int(_filteredElements.size()));
		while (clipper.Step())
		{
			for (int lineIdx = clipper.DisplayStart;
				lineIdx < clipper.DisplayEnd; lineIdx++)
			{
				ImGui::TableNextRow();

				// the line selection/highlight
				ImGui::TableNextColumn();
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DIS_BG_CLR_BRK);
				const bool isSelected = m_selectedLineIdx == lineIdx;
				if (ImGui::Selectable(
					std::format("##ElementLineId{}",lineIdx).c_str(),
					isSelected,
					ImGuiSelectableFlags_SpanAllColumns))
				{
					m_selectedLineIdx = lineIdx;
				}
				hoveredLineIdx = ImGui::IsItemHovered() ? lineIdx : hoveredLineIdx;

				const auto& [label, addr, addrS] = _filteredElements.at(lineIdx);

				// draw the element
				ImGui::TableNextColumn();
				ImGui::TextColored( label[0] == '@' ?
					DASM_CLR_LABEL_LOCAL : DASM_CLR_LABEL_GLOBAL,
					label.c_str());

				// draw the addr
				ImGui::TableNextColumn();
				ImGui::TextColored( DASM_CLR_ADDR, addrS.c_str());

				// double-click to locate the addr
				if (hoveredLineIdx >= 0)
				{
					if ( ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						const auto& [element, selectedAddr, selectedAddrS] =
								_filteredElements.at(m_selectedLineIdx);

						if (ImGui::GetIO().KeyCtrl)
						{
							m_scheduler.AddSignal(
								{dev::Signals::HEX_HIGHLIGHT_ON,
								dev::Scheduler::GlobalAddrLen(selectedAddr, 1)});
						}
						else{
							m_scheduler.AddSignal(
								{dev::Signals::DISASM_UPDATE, (Addr)selectedAddr});
						}
					}

					// init the context menu
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
					{
						const auto& [element, selectedAddr, selectedAddrS] =
							_filteredElements.at(hoveredLineIdx);
						m_contextMenu.Init(selectedAddr, element, _elementType);
					}
				}

			}
		}
		ImGui::EndTable();
		// init the context menu
		ImVec2 tableMin = ImGui::GetItemRectMin();
		ImVec2 tableMax = ImGui::GetItemRectMax();
		if (ImGui::IsMouseHoveringRect(tableMin, tableMax) &&
			hoveredLineIdx < 0 &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			m_contextMenu.Init(0, "", _elementType, false);
		}
	}

	DrawContextMenu(m_contextMenu);
}

void dev::DebugDataWindow::DrawContextMenu(ContextMenu& _contextMenu)
{
	if (_contextMenu.BeginPopup())
	{
		if (_contextMenu.itemHovered)
		{
			if (ImGui::MenuItem("Copy Name")) {
				dev::CopyToClipboard(_contextMenu.elementName);
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Copy Addr")) {
				dev::CopyToClipboard(std::format("0x{:X}", (_contextMenu.addr)));
				ImGui::CloseCurrentPopup();
			}

			ImGui::SeparatorText("");

			if (ImGui::MenuItem("Locate in the Disasm Window"))
			{
				m_scheduler.AddSignal(
					{dev::Signals::DISASM_UPDATE, (Addr)_contextMenu.addr});
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Locate in the Hex WIndow"))
			{
				m_scheduler.AddSignal(
					{dev::Signals::HEX_HIGHLIGHT_ON,
					dev::Scheduler::GlobalAddrLen(_contextMenu.addr, 1)});
				ImGui::CloseCurrentPopup();
			}

			ImGui::SeparatorText("");
		}

		if (ImGui::MenuItem("Add"))
		{
			ImGui::CloseCurrentPopup();

			dev::Signals signals = dev::Signals::NONE;

			switch (_contextMenu.elementType)
			{
			case ElementType::LABEL:
				signals = dev::Signals::LABEL_EDIT_WINDOW_ADD;
				break;
			case ElementType::CONST:
				signals = dev::Signals::CONST_EDIT_WINDOW_ADD;
				break;
			case ElementType::COMMENT:
				signals = dev::Signals::COMMENT_EDIT_WINDOW_ADD;
				break;
			case ElementType::MEMORY_EDIT:
				signals = dev::Signals::MEMORY_EDIT_WINDOW_ADD;
				break;
			case ElementType::CODE_PERFS:
				signals = dev::Signals::CODE_PERF_EDIT_WINDOW_ADD;
				break;
			case ElementType::SCRIPTS:
				signals = dev::Signals::SCRIPT_EDIT_WINDOW_ADD;
				break;
			}
			m_scheduler.AddSignal({signals, (Addr)0});
		}

		if (_contextMenu.itemHovered)
		{
			if (ImGui::MenuItem("Edit"))
			{
				ImGui::CloseCurrentPopup();

				dev::Signals signals = dev::Signals::NONE;

				switch (_contextMenu.elementType)
				{
				case ElementType::LABEL:
					signals = dev::Signals::LABEL_EDIT_WINDOW_EDIT;
					break;
				case ElementType::CONST:
					signals = dev::Signals::CONST_EDIT_WINDOW_EDIT;
					break;
				case ElementType::COMMENT:
					signals = dev::Signals::COMMENT_EDIT_WINDOW_EDIT;
					break;
				case ElementType::MEMORY_EDIT:
					signals = dev::Signals::MEMORY_EDIT_WINDOW_EDIT;
					break;
				case ElementType::CODE_PERFS:
					signals = dev::Signals::CODE_PERF_EDIT_WINDOW_EDIT;
					break;
				case ElementType::SCRIPTS:
					signals = dev::Signals::SCRIPT_EDIT_WINDOW_EDIT;
					break;
				}
				m_scheduler.AddSignal({signals, (Addr)_contextMenu.addr});
			}

			if (ImGui::MenuItem("Delete"))
			{
				switch (_contextMenu.elementType)
				{
				case ElementType::LABEL:
					m_debugger.GetDebugData().DelLabel(
						_contextMenu.addr, _contextMenu.elementName);
					break;

				case ElementType::CONST:
					m_debugger.GetDebugData().DelConst(
						_contextMenu.addr, _contextMenu.elementName);
					break;

				case ElementType::COMMENT:
					m_debugger.GetDebugData().DelComment(_contextMenu.addr);
					break;

				case ElementType::MEMORY_EDIT:
					m_debugger.GetDebugData().DelMemoryEdit(_contextMenu.addr);
					break;

				case ElementType::CODE_PERFS:
					m_debugger.GetDebugData().DelCodePerf(_contextMenu.addr);
					break;
				case ElementType::SCRIPTS:
					m_debugger.GetDebugData().GetScripts().Del(_contextMenu.addr);
					break;

				default:
					break;
				}
				m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
				ImGui::CloseCurrentPopup();
			}

			ImGui::SeparatorText("");
		}

		if (ImGui::MenuItem("Delete All"))
		{
			switch (_contextMenu.elementType)
			{
			case ElementType::LABEL:
				m_debugger.GetDebugData().DelAllLabels();
				break;

			case ElementType::CONST:
				m_debugger.GetDebugData().DelAllConsts();
				break;

			case ElementType::COMMENT:
				m_debugger.GetDebugData().DelAllComments();
				break;

			case ElementType::MEMORY_EDIT:
				m_hardware.Request(Hardware::Req::DEBUG_MEMORY_EDIT_DEL_ALL);
				break;

			case ElementType::CODE_PERFS:
				m_hardware.Request(Hardware::Req::DEBUG_CODE_PERF_DEL_ALL);
				break;
			case ElementType::SCRIPTS:
				m_hardware.Request(Hardware::Req::DEBUG_SCRIPT_DEL_ALL);
				break;

			default:
				break;
			}

			m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}
