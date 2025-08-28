#include <format>
#include "ui/watchpoints_window.h"
#include "utils/str_utils.h"

dev::WatchpointsWindow::WatchpointsWindow(Hardware& _hardware,
	dev::Scheduler& _scheduler,
	bool* _visibleP)
	:
	BaseWindow("Watchpoints", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP),
	m_hardware(_hardware)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::WATCHPOINTS,
			std::bind(&dev::WatchpointsWindow::UpdateWatchpoints,
				this, std::placeholders::_1, std::placeholders::_2),
			m_visibleP));
}

void dev::WatchpointsWindow::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	// Draw a table
	static int selectedAddr = 0;
	bool showItemContextMenu = false;
	static int editedWatchpointId = -1;
	const int COLUMNS_COUNT = 5;

	const char* tableName = "##Watchpoints";
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
		auto scale = ImGui::GetWindowDpiScale();

		ImGui::TableSetupColumn(
			"###WpActive", ImGuiTableColumnFlags_WidthFixed, 25 * scale);
		ImGui::TableSetupColumn(
			"GlobalAddr", ImGuiTableColumnFlags_WidthFixed, 110 * scale);
		ImGui::TableSetupColumn(
			"Access", ImGuiTableColumnFlags_WidthFixed, 50 * scale);
		ImGui::TableSetupColumn(
			"Condition", ImGuiTableColumnFlags_WidthFixed, 110 * scale);
		ImGui::TableSetupColumn(
			"Comment", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
		for (int column = 0; column < COLUMNS_COUNT; column++)
		{
			ImGui::TableSetColumnIndex(column);
			// Retrieve the name passed to TableSetupColumn()
			const char* column_name = ImGui::TableGetColumnName(column);
			if (column == 0) {
				DrawHelpMarker(
					"Define address(es) and conditions; halts execution\n"
					"when memory is accessed under specified conditions\n\n"

					"Left-click in empty space: Open context menu\n"
					"Left-click on item: Open item context menu\n"
					"Double left-click in empty space: Create a new watchpoint");
			}
			else {
				ImGui::TableHeader(column_name);
			}
		}

		PushStyleCompact(1.0f, 0.0f);

		for (auto& [id, wp] : m_watchpoints)
		{
			ImGui::TableNextRow(ImGuiTableRowFlags_None, 21.0f);
			auto globalAddr = wp.data.globalAddr;
			// isActive
			ImGui::TableNextColumn();

			auto isActive = wp.data.active;
			ImGui::Checkbox(
				std::format("##{:05X}", globalAddr).c_str(), &isActive);

			if (isActive != wp.data.active)
			{
				wp.data.active = isActive;
				m_hardware.Request(Hardware::Req::DEBUG_WATCHPOINT_ADD, {
					{"data0", wp.data.data0},
					{"data1", wp.data.data1},
					{"comment", wp.comment} });

				m_scheduler.AddSignal(
					{dev::Signals::HEX_HIGHLIGHT_ON,
					Scheduler::GlobalAddrLen{
						(GlobalAddr)globalAddr, (uint16_t)wp.data.len}});
			}
			// GlobalAddr
			ImGui::TableNextColumn();
			ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
			const bool isSelected = selectedAddr == (int)globalAddr;
			if (ImGui::Selectable(
					std::format("0x{:05X}",
					globalAddr).c_str(),
					isSelected,
					ImGuiSelectableFlags_SpanAllColumns))
			{
				selectedAddr = globalAddr;
				m_scheduler.AddSignal(
					{dev::Signals::HEX_HIGHLIGHT_ON,
					Scheduler::GlobalAddrLen{
						(GlobalAddr)selectedAddr, (uint16_t)wp.data.len}});
			}

			ImVec2 rowMin = ImGui::GetItemRectMin();
			CheckIfItemClicked(
				rowMin, showItemContextMenu, id, editedWatchpointId);
			ImGui::PopStyleColor();

			// Access
			DrawProperty(wp.GetAccessS());
			CheckIfItemClicked(
				rowMin, showItemContextMenu, id, editedWatchpointId);

			// Condition
			std::string condS = wp.GetConditionS();
			if (wp.data.cond != dev::Condition::ANY) {
				if (wp.data.type == Watchpoint::Type::LEN)
				{
					condS += std::format("{:02X}", wp.data.value);
				}
				else {
					condS += std::format("{:04X}", wp.data.value);
				}
			}
			condS = std::format("{} l:{}", condS, wp.data.len);
			DrawProperty(condS);
			CheckIfItemClicked(
				rowMin, showItemContextMenu, id, editedWatchpointId);


			// Comment
			DrawProperty(wp.GetComment());
			CheckIfItemClicked(
				rowMin, showItemContextMenu, id, editedWatchpointId);

			// Show the tooltip if the item is hovered
			ImVec2 rowMax = ImGui::GetItemRectMax();
			if (ImGui::IsMouseHoveringRect(rowMin, rowMax, false) &&
				!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
			{
				// convert the watchpoint memory data to a string
				auto addr = wp.data.globalAddr;
				auto len = wp.data.len;
				auto data = m_hardware.Request(
					Hardware::Req::GET_MEM_STRING_GLOBAL,
					{{"addr", addr}, {"len", len}});
				auto hexS = data->at("data").get<std::string>();


				// draw a tooltip
				ImGui::BeginTooltip();
				ImGui::Text(hexS.c_str());
				ImGui::EndTooltip();
			}
		}

		PopStyleCompact();
		ImGui::EndTable();

		// double-click to add a new item
		ImVec2 tableMin = ImGui::GetItemRectMin();
		ImVec2 tableMax = ImGui::GetItemRectMax();
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
			!ImGui::IsAnyItemHovered())
			if (ImGui::IsMouseHoveringRect(tableMin, tableMax))
		{
			m_scheduler.AddSignal({dev::Signals::WATCHPOINTS_POPUP_ADD});
		}

		// the context menu
		if (ImGui::BeginPopupContextItem("WpContextMenu",
			ImGuiPopupFlags_NoOpenOverItems |
			ImGuiPopupFlags_NoOpenOverExistingPopup |
			ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Add")) {
				m_scheduler.AddSignal({dev::Signals::WATCHPOINTS_POPUP_ADD});
			}
			else if (ImGui::MenuItem("Disable All"))
			{
				for (auto& [id, wp] : m_watchpoints)
				{
					wp.data.active = false;
					m_hardware.Request(Hardware::Req::DEBUG_WATCHPOINT_ADD,
						{ {"data0", wp.data.data0},
						{"data1", wp.data.data1},
						{"comment", wp.comment} });
				}
			}
			else if (ImGui::MenuItem("Delete All")) {
				m_hardware.Request(Hardware::Req::DEBUG_WATCHPOINT_DEL_ALL);
				m_scheduler.AddSignal({dev::Signals::HEX_HIGHLIGHT_OFF});
			};
			ImGui::EndPopup();
		}

		// the item context menu
		if (showItemContextMenu) ImGui::OpenPopup("WpItemMenu");
		if (ImGui::BeginPopup("WpItemMenu"))
		{
			if (editedWatchpointId >= 0)
			{
				auto& wp = m_watchpoints.at(editedWatchpointId);

				if (ImGui::MenuItem(wp.data.active ? "Disable" : "Enable"))
				{
					wp.data.active = !wp.data.active;
					m_hardware.Request(
						Hardware::Req::DEBUG_WATCHPOINT_ADD, {
							{"data0", wp.data.data0},
							{"data1", wp.data.data1},
							{"comment", wp.comment} });

					m_scheduler.AddSignal(
						{dev::Signals::HEX_HIGHLIGHT_ON,
						Scheduler::GlobalAddrLen{
							(GlobalAddr)wp.data.globalAddr,
							(uint16_t)wp.data.len}});
				}
				if (ImGui::MenuItem("Delete"))
				{
					m_hardware.Request(
						Hardware::Req::DEBUG_WATCHPOINT_DEL,
						{ {"id", editedWatchpointId} });

					m_scheduler.AddSignal({dev::Signals::HEX_HIGHLIGHT_OFF});
				}
				else if (ImGui::MenuItem("Edit"))
				{
					m_scheduler.AddSignal({
						dev::Signals::WATCHPOINTS_POPUP_EDIT,
						dev::Id(editedWatchpointId)});
				};
			}

			ImGui::EndPopup();
		}
	}
	ImGui::PopStyleVar(2);
}

void dev::WatchpointsWindow::DrawProperty(
	const std::string& _name, const ImVec2& _aligment)
{
	ImGui::TableNextColumn();
	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name.c_str(), _aligment);
	ImGui::PopStyleColor();
}

void dev::WatchpointsWindow::CheckIfItemClicked(const ImVec2& _rowMin,
	bool& _showItemContextMenu,
	const int _id,
	int& _editedWatchpointId)
{
	if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
	{
		_showItemContextMenu = true;
		_editedWatchpointId = _id;
	}

	// if double clicked, open the wp edit window
	if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		ImVec2 rowMax = ImGui::GetItemRectMax();

		if (ImGui::IsMouseHoveringRect(_rowMin, rowMax))
		{
			_editedWatchpointId = _id;
			m_scheduler.AddSignal({
				dev::Signals::WATCHPOINTS_POPUP_EDIT,
				dev::Id(_editedWatchpointId)});
		}
	}
}


void dev::WatchpointsWindow::UpdateWatchpoints(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	m_watchpoints.clear();
	auto watchpointsJ = m_hardware.Request(
		Hardware::Req::DEBUG_WATCHPOINT_GET_ALL);

	if (watchpointsJ)
	{
		for (const auto& watchpointJ : *watchpointsJ)
		{
			Watchpoint::Data wpData{ watchpointJ["data0"], watchpointJ["data1"] };

			Watchpoint wp{ std::move(wpData), watchpointJ["comment"] };
			auto id = wp.data.id;
			m_watchpoints.emplace(id, std::move(wp));
		}
	}

}