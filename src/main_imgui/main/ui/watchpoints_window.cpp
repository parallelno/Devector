#include <format>
#include "ui/watchpoints_window.h"
#include "utils/str_utils.h"

dev::WatchpointsWindow::WatchpointsWindow(Hardware& _hardware,
	const float* const _dpiScaleP, ReqUI& _reqUI)
	:
	BaseWindow("Watchpoints", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _dpiScaleP),
	m_hardware(_hardware),
	m_reqUI(_reqUI)
{}

void dev::WatchpointsWindow::Update(bool& _visible, const bool _isRunning)
{
	BaseWindow::Update();

	if (_visible && ImGui::Begin(m_name.c_str(), &_visible, ImGuiWindowFlags_NoCollapse))
	{
		DrawTable();
		ImGui::End();
	}
}

void dev::WatchpointsWindow::DrawProperty(const std::string& _name, const ImVec2& _aligment)
{
	ImGui::TableNextColumn();
	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name.c_str(), _aligment);
	ImGui::PopStyleColor();
}

void dev::WatchpointsWindow::CheckIfItemClicked(const ImVec2& _rowMin, 
	bool& _showItemContextMenu, const int _id, int& _editedWatchpointId, ReqPopup& _reqPopup)
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
			_reqPopup = ReqPopup::INIT_EDIT;
		}
	}
}

void dev::WatchpointsWindow::DrawTable()
{
	static int selectedAddr = 0;
	bool showItemContextMenu = false;
	static int editedWatchpointId = -1;
	static ReqPopup reqPopup = ReqPopup::NONE;
	const int COLUMNS_COUNT = 5;

	const char* tableName = "##Watchpoints";
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 0.0f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });

	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Hideable;
	if (ImGui::BeginTable(tableName, COLUMNS_COUNT, flags))
	{
		ImGui::TableSetupColumn("###WpActive", ImGuiTableColumnFlags_WidthFixed, 25);
		ImGui::TableSetupColumn("GlobalAddr", ImGuiTableColumnFlags_WidthFixed, 110);
		ImGui::TableSetupColumn("Access", ImGuiTableColumnFlags_WidthFixed, 50);
		ImGui::TableSetupColumn("Condition", ImGuiTableColumnFlags_WidthFixed, 110);
		ImGui::TableSetupColumn("Comment", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
		for (int column = 0; column < COLUMNS_COUNT; column++)
		{
			ImGui::TableSetColumnIndex(column);
			const char* column_name = ImGui::TableGetColumnName(column); // Retrieve the name passed to TableSetupColumn()
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

		UpdateWatchpoints();

		for (auto& [id, wp] : m_watchpoints)
		{
			ImGui::TableNextRow(ImGuiTableRowFlags_None, 21.0f);
			auto globalAddr = wp.data.globalAddr;
			// isActive
			ImGui::TableNextColumn();

			auto isActive = wp.data.active;
			ImGui::Checkbox(std::format("##{:05X}", globalAddr).c_str(), &isActive);
			if (isActive != wp.data.active)
			{
				wp.data.active = isActive;
				m_hardware.Request(Hardware::Req::DEBUG_WATCHPOINT_ADD,
					{ {"data0", wp.data.data0}, {"data1", wp.data.data1}, {"comment", wp.comment} });

				m_reqUI.type = ReqUI::Type::HEX_HIGHLIGHT_ON;
				m_reqUI.globalAddr = globalAddr;
				m_reqUI.len = wp.data.len;
			}
			// GlobalAddr
			ImGui::TableNextColumn();
			ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
			const bool isSelected = selectedAddr == (int)globalAddr;
			if (ImGui::Selectable(std::format("0x{:05X}", globalAddr).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
			{
				selectedAddr = globalAddr;
				m_reqUI.type = ReqUI::Type::HEX_HIGHLIGHT_ON;
				m_reqUI.globalAddr = selectedAddr;
				m_reqUI.len = wp.data.len;
			}
			ImVec2 rowMin = ImGui::GetItemRectMin();
			CheckIfItemClicked(rowMin, showItemContextMenu, id, editedWatchpointId, reqPopup);
			ImGui::PopStyleColor();

			// Access
			DrawProperty(wp.GetAccessS());
			CheckIfItemClicked(rowMin, showItemContextMenu, id, editedWatchpointId, reqPopup);

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
			CheckIfItemClicked(rowMin, showItemContextMenu, id, editedWatchpointId, reqPopup);


			// Comment
			DrawProperty(wp.GetComment());
			CheckIfItemClicked(rowMin, showItemContextMenu, id, editedWatchpointId, reqPopup);
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
			reqPopup = ReqPopup::INIT_ADD;
		}

		// the context menu
		if (ImGui::BeginPopupContextItem("WpContextMenu",
			ImGuiPopupFlags_NoOpenOverItems |
			ImGuiPopupFlags_NoOpenOverExistingPopup |
			ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Add New")) {
				reqPopup = ReqPopup::INIT_ADD;
			}
			else if (ImGui::MenuItem("Disable All"))
			{
				for (auto& [id, wp] : m_watchpoints) 
				{
					wp.data.active = false;
					m_hardware.Request(Hardware::Req::DEBUG_WATCHPOINT_ADD,
						{ {"data0", wp.data.data0}, {"data1", wp.data.data1}, {"comment", wp.comment} });
				}
			}
			else if (ImGui::MenuItem("Delete All")) {
				m_hardware.Request(Hardware::Req::DEBUG_WATCHPOINT_DEL_ALL);
				m_reqUI.type = ReqUI::Type::HEX_HIGHLIGHT_OFF;
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
					m_hardware.Request(Hardware::Req::DEBUG_WATCHPOINT_ADD,
						{ {"data0", wp.data.data0}, {"data1", wp.data.data1}, {"comment", wp.comment} });

					m_reqUI.type = ReqUI::Type::HEX_HIGHLIGHT_ON;
					m_reqUI.globalAddr = wp.data.globalAddr;
					m_reqUI.len = wp.data.len;
				}
				if (ImGui::MenuItem("Delete")) 
				{
					m_hardware.Request(Hardware::Req::DEBUG_WATCHPOINT_DEL, { {"id", editedWatchpointId} });
					m_reqUI.type = ReqUI::Type::HEX_HIGHLIGHT_OFF;
				}
				else if (ImGui::MenuItem("Edit")) {
					reqPopup = ReqPopup::INIT_EDIT;
				};
			}

			ImGui::EndPopup();
		}

		DrawPopup(reqPopup, m_watchpoints, editedWatchpointId);

	}
	ImGui::PopStyleVar(2);
}

void dev::WatchpointsWindow::DrawPopup(ReqPopup& _reqPopup, const Watchpoints::WpMap& _wps, int _id)
{
	if (_reqPopup == ReqPopup::NONE) {
		return;
	}
	else if (_reqPopup == ReqPopup::INIT_ADD || _reqPopup == ReqPopup::INIT_EDIT) {
		ImGui::OpenPopup("##WpEdit");
	}

	static bool isActive = true;
	static int oldId = -1;
	static std::string globalAddrS = "FF";
	static int access = static_cast<int>(Watchpoint::Access::RW);
	static int cond = static_cast<int>(dev::Condition::ANY);
	static std::string valueS = "";
	static int type = static_cast<int>(Watchpoint::Type::LEN);
	static std::string lenS = "FF";
	static std::string commentS = "";
	static ImVec2 buttonSize = { 65.0f, 25.0f };

	// Init for a new WP
	if (_reqPopup == ReqPopup::INIT_ADD) {
		_reqPopup = ReqPopup::ADD;
		commentS = "";
		globalAddrS = "FF";
	}

	// Init for editing WP
	if (_reqPopup == ReqPopup::INIT_EDIT)
	{
		_reqPopup = ReqPopup::EDIT;

		const auto& wp = _wps.at(_id);
		oldId = _id;
		isActive = wp.data.active;
		globalAddrS = std::format("{:04X}", wp.data.globalAddr);
		access = wp.GetAccessI();
		cond = static_cast<int>(wp.data.cond);
		type = static_cast<int>(wp.data.type);
		if (type == static_cast<int>(Watchpoint::Type::WORD)) {
			valueS = std::format("{:04X}", wp.data.value);
		}
		else {
			valueS = std::format("{:02X}", wp.data.value);
		}
		
		lenS = std::format("{:04X}", wp.data.len);
		commentS = wp.GetComment();
	}

	if (ImGui::BeginPopup("##WpEdit"))
	{
		static ImGuiTableFlags flags =
			ImGuiTableFlags_ScrollY |
			ImGuiTableFlags_SizingStretchSame |
			ImGuiTableFlags_ContextMenuInBody;

		if (ImGui::BeginTable("##WpContextMenu", 2, flags))
		{
			ImGui::TableSetupColumn("##WpContextMenuName", ImGuiTableColumnFlags_WidthFixed, 150);
			ImGui::TableSetupColumn("##WpContextMenuVal", ImGuiTableColumnFlags_WidthFixed, 200);
			// Status
			DrawProperty2EditableCheckBox("Active", "##WpContextStatus", &isActive);
			// addr
			DrawProperty2EditableS("Global Address", "##WpContextAddress", &globalAddrS, "FF",
				"A hexademical address in the format 0xFF or FF");
			// Access
			/*DrawProperty2Access("Access", &access, "R - read, W - write, RW - read or write");*/
			DrawProperty2RadioButtons("Type", &access, dev::wpAccessS, IM_ARRAYSIZE(dev::wpAccessS), 8.0f,
				"R - read, W - write, RW - read or write");

			// Condition
			DrawProperty2Combo("Condition", "##WpContextCondition", 
				&cond, dev::ConditionsS, IM_ARRAYSIZE(dev::ConditionsS), "");

			// Value
			if (cond == static_cast<int>(dev::Condition::ANY)) ImGui::BeginDisabled();
			DrawProperty2EditableS("Value", "##WpContextValue", &valueS, 
				type == static_cast<int>(Watchpoint::Type::LEN) ? "FF" : "FFFF",
				"A hexademical value in the format 0xFF or FF");
			if (cond == static_cast<int>(dev::Condition::ANY)) ImGui::EndDisabled();

			// Type
			DrawProperty2RadioButtons("Type", &type, wpTypesS, IM_ARRAYSIZE(wpTypesS), 15.0f,
				"Byte - breaks if the condition succeeds for any bytes in the defined range\n"
				"Word - breaks if the condition succeeds for a word");
			
			// Length
			if (type == static_cast<int>(Watchpoint::Type::WORD)) {
				ImGui::BeginDisabled();
				lenS = "2";
			}
			DrawProperty2EditableS("Length", "##WpContextLen", &lenS, "FFFF", 
				"A hexademical value in the format FF", 
				ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
			if (type == static_cast<int>(Watchpoint::Type::WORD)) ImGui::EndDisabled();

			// Comment
			DrawProperty2EditableS("Comment", "##WpContextComment", &commentS, "");

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			
			// Warnings
			std::string warningS = "";

			GlobalAddr globalAddr = dev::StrHexToInt(globalAddrS.c_str());
			if (globalAddr >= Memory::GLOBAL_MEMORY_LEN) {
				warningS = "Too large address";
			}
			if (lenS.size() > 4) {
				warningS = "Too large length";
			}
			ImGui::TextColored(COLOR_WARNING, warningS.c_str());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();

			// OK button
			if (!warningS.empty()) ImGui::BeginDisabled();
			if (ImGui::Button("Ok", buttonSize))
			{
				int id = _reqPopup == ReqPopup::ADD ? -1 : oldId;
				GlobalAddr len = dev::StrHexToInt(lenS.c_str());
				uint16_t value = dev::StrHexToInt(valueS.c_str());

				Watchpoint::Data wpData{ id, static_cast<Watchpoint::Access>(access),
					globalAddr, static_cast<dev::Condition>(cond),
					value, static_cast<Watchpoint::Type>(type),
					len, isActive };

				m_hardware.Request(Hardware::Req::DEBUG_WATCHPOINT_ADD,
					{ {"data0", wpData.data0}, {"data1", wpData.data1}, {"comment", commentS} });

				m_reqUI.type = ReqUI::Type::HEX_HIGHLIGHT_ON;
				m_reqUI.globalAddr = globalAddr;
				m_reqUI.len = len;
				ImGui::CloseCurrentPopup();
			}
			if (!warningS.empty()) ImGui::EndDisabled();

			// Cancel button
			ImGui::SameLine(); ImGui::Text(" "); ImGui::SameLine();
			if (ImGui::Button("Cancel", buttonSize)) ImGui::CloseCurrentPopup();

			ImGui::EndTable();
		}

		ImGui::EndPopup();
	}
}

void dev::WatchpointsWindow::UpdateWatchpoints()
{
	size_t updates = m_hardware.Request(Hardware::Req::DEBUG_WATCHPOINT_GET_UPDATES)->at("updates");

	if (updates == m_updates) return;

	m_updates = updates;

	m_watchpoints.clear();
	auto watchpointsJ = m_hardware.Request(Hardware::Req::DEBUG_WATCHPOINT_GET_ALL);
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