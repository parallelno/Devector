#include <format>

#include "ui/breakpoints_window.h"
#include "utils/types.h"
#include "utils/consts.h"
#include "utils/str_utils.h"

dev::BreakpointsWindow::BreakpointsWindow(Hardware& _hardware,
	dev::Scheduler& _scheduler,
	bool* _visibleP, const float* const _dpiScaleP)
	:
	BaseWindow("Breakpoints", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP, _dpiScaleP),
	m_hardware(_hardware)
{
	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::BREAKPOINTS,
			std::bind(&dev::BreakpointsWindow::CallbackUpdateBreakpoints,
				this, std::placeholders::_1, std::placeholders::_2),
			m_visibleP));
}

void dev::BreakpointsWindow::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	// Draw a table
	static int selectedAddr = -1;
	bool showItemContextMenu = false;
	static int editedBreakpointAddr = -1;
	const int COLUMNS_COUNT = 4;

	const char* tableName = "##Breakpoints";
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 0.0f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });

	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadInnerX |
		ImGuiTableFlags_NoPadOuterX |
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_BordersOuter |
		ImGuiTableFlags_Hideable;
	if (ImGui::BeginTable(tableName, COLUMNS_COUNT, flags))
	{
		ImGui::TableSetupColumn("##BPActive", ImGuiTableColumnFlags_WidthFixed, 25);
		ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed, 110);
		ImGui::TableSetupColumn("Condition", ImGuiTableColumnFlags_WidthFixed, 180);
		ImGui::TableSetupColumn("Comment", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
		for (int column = 0; column < COLUMNS_COUNT; column++)
		{
			ImGui::TableSetColumnIndex(column);
			const char* column_name = ImGui::TableGetColumnName(column); // Retrieve the name passed to TableSetupColumn()
			if (column == 0) {
				DrawHelpMarker(
					"Define an address and conditions; halts execution when the\n"
					"program counter reaches it under specified conditions\n\n"

					"Left-click in empty space: Open context menu\n"
					"Left-click on item: Open item context menu\n"
					"Double left-click in empty space: Create a new breakpoint");
			}
			else {
				ImGui::TableHeader(column_name);
			}
		}

		PushStyleCompact(1.0f, 0.0f);

		for (const auto& [addr, bp] : m_breakpoints)
		{
			ImGui::TableNextRow(ImGuiTableRowFlags_None, 21.0f);

			// isActive
			ImGui::TableNextColumn();

			bool isActive =
				bp.data.structured.status == Breakpoint::Status::ACTIVE;
			ImGui::Checkbox(std::format("##0x{:04X}", addr).c_str(), &isActive);
			Breakpoint::Status newStatus = isActive ?
				Breakpoint::Status::ACTIVE : Breakpoint::Status::DISABLED;

			if (newStatus != bp.data.structured.status)
			{
				m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_SET_STATUS,
					{ {"addr", addr},
					{"status", static_cast<uint8_t>(newStatus)}});
			}
			// Addr
			ImGui::TableNextColumn();
			ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
			const bool isSelected = selectedAddr == (int)addr;
			if (ImGui::Selectable(bp.GetAddrMappingS(),
				isSelected,
				ImGuiSelectableFlags_SpanAllColumns |
				ImGuiSelectableFlags_AllowDoubleClick))
			{
				selectedAddr = addr;
				m_scheduler.AddSignal(
					{dev::Signals::DISASM_UPDATE, (GlobalAddr)addr});
			}
			ImVec2 rowMin = ImGui::GetItemRectMin();
			CheckIfItemClicked(rowMin, showItemContextMenu, addr,
				editedBreakpointAddr);

			ImGui::PopStyleColor();

			// Condition
			std::string cond = bp.GetConditionS();
			DrawProperty(cond);
			CheckIfItemClicked(rowMin, showItemContextMenu, addr,
				editedBreakpointAddr);

			// Comment
			DrawProperty(bp.comment);
			CheckIfItemClicked(rowMin, showItemContextMenu, addr,
				editedBreakpointAddr);
		}

		PopStyleCompact();
		ImGui::EndTable();

		// double-click to add a new item
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
			!ImGui::IsAnyItemHovered())
		{
			ImVec2 tableMin = ImGui::GetItemRectMin();
			ImVec2 tableMax = ImGui::GetItemRectMax();
			if (ImGui::IsMouseHoveringRect(tableMin, tableMax))
			{
				m_scheduler.AddSignal({dev::Signals::BREAKPOINTS_POPUP_ADD});
			}
		}
		// the context menu
		if (ImGui::BeginPopupContextItem("BpContextMenu",
			ImGuiPopupFlags_NoOpenOverItems |
			ImGuiPopupFlags_NoOpenOverExistingPopup |
			ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Add")) {
				m_scheduler.AddSignal({dev::Signals::BREAKPOINTS_POPUP_ADD});
			}
			else if (ImGui::MenuItem("Disable All"))
			{
				for (const auto& [addr, bp] : m_breakpoints) {
					m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DISABLE,
						{ {"addr", addr} });
				}
			}
			else if (ImGui::MenuItem("Delete All"))
			{
				m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DEL_ALL);
			}
			ImGui::EndPopup();
		}

		// the item context menu
		if (showItemContextMenu) ImGui::OpenPopup("BpItemMenu");
		if (ImGui::BeginPopup("BpItemMenu"))
		{
			if (editedBreakpointAddr >= 0)
			{
				const auto& bp = m_breakpoints.at(editedBreakpointAddr);

				if (bp.IsActive()) {
					if (ImGui::MenuItem("Disable")) {
						m_hardware.Request(
							Hardware::Req::DEBUG_BREAKPOINT_DISABLE,
							{ {"addr", editedBreakpointAddr} });

					}
				}
				else {
					if (ImGui::MenuItem("Enable")) {
						m_hardware.Request(
							Hardware::Req::DEBUG_BREAKPOINT_ACTIVE,
							{ {"addr", editedBreakpointAddr} });
					}
				}
				if (ImGui::MenuItem("Delete")) {
					m_hardware.Request(
						Hardware::Req::DEBUG_BREAKPOINT_DEL,
						{ {"addr", editedBreakpointAddr}});
				}
				else if (ImGui::MenuItem("Edit"))
				{
					m_scheduler.AddSignal({
						dev::Signals::BREAKPOINTS_POPUP_EDIT,
						dev::GlobalAddr(editedBreakpointAddr)});
				};
			}
			ImGui::EndPopup();
		}
	}

	ImGui::PopStyleVar(2);
}

void dev::BreakpointsWindow::DrawProperty(
	const std::string& _name, const ImVec2& _aligment)
{
	ImGui::TableNextColumn();
	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name.c_str(), _aligment);
	ImGui::PopStyleColor();
}

void dev::BreakpointsWindow::CheckIfItemClicked(
	const ImVec2& _rowMin, bool& _showItemContextMenu,
	const int _addr, int& _editedBreakpointAddr)
{
	if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
	{
		_showItemContextMenu = true;
		_editedBreakpointAddr = _addr;
	}

	// if double clicked, open the wp edit window
	if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		ImVec2 rowMax = ImGui::GetItemRectMax();

		if (ImGui::IsMouseHoveringRect(_rowMin, rowMax))
		{
			_editedBreakpointAddr = _addr;
			m_scheduler.AddSignal({
				dev::Signals::BREAKPOINTS_POPUP_EDIT,
				dev::GlobalAddr(_editedBreakpointAddr)});
		}
	}
}


void dev::BreakpointsWindow::CallbackUpdateBreakpoints(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	m_breakpoints.clear();
	auto breakpointsJ = m_hardware.Request(
		Hardware::Req::DEBUG_BREAKPOINT_GET_ALL);

	if (breakpointsJ)
	{
		for (const auto& breakpointJ : *breakpointsJ)
		{
			Breakpoint::Data bpData{
				breakpointJ["data0"],
				breakpointJ["data1"],
				breakpointJ["data2"]
			};

			Breakpoint bp{ std::move(bpData), breakpointJ["comment"] };
			auto addr = bp.data.structured.addr;
			m_breakpoints.emplace(addr, std::move(bp));
		}
	}

}