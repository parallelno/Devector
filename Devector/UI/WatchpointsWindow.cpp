#include <format>
#include "WatchpointsWindow.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/StrUtils.h"

dev::WatchpointsWindow::WatchpointsWindow(Debugger& _debugger,
	const float* const _fontSizeP, const float* const _dpiScaleP, ReqHexViewer& _reqHexViewer)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_debugger(_debugger),
	m_reqHexViewer(_reqHexViewer)
{}

void dev::WatchpointsWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Watchpoints", &open, ImGuiWindowFlags_NoCollapse);

	DrawTable();

	ImGui::End();
}

void dev::WatchpointsWindow::DrawProperty(const std::string& _name, const ImVec2& _aligment)
{
	ImGui::TableNextColumn();
	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name.c_str(), _aligment);
	ImGui::PopStyleColor();
}

void dev::WatchpointsWindow::DrawProperty2Access(
	const char* _name, int* _access, const char* _hint)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();

	ImGui::RadioButton("R ###WpAccessR", _access, 0); ImGui::SameLine();
	ImGui::RadioButton("W ###WpAccessW", _access, 1); ImGui::SameLine();
	ImGui::RadioButton("RW ###WpAccessRW", _access, 2);

	if (*_hint != '\0') {
		ImGui::SameLine();
		DrawHelpMarker(_hint);
	}
}

void dev::WatchpointsWindow::DrawProperty2Type(
	const char* _name, int* _type, const char* _hint)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	
	ImGui::RadioButton("byte ###WpSizeB", _type, 0); ImGui::SameLine();
	ImGui::RadioButton("word ###WpSizeW", _type, 1);

	if (*_hint != '\0') {
		ImGui::SameLine();
		DrawHelpMarker(_hint);
	}
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

		auto watchpoints = m_debugger.GetWatchpoints();

		for (const auto& [id, wp] : watchpoints)
		{
			ImGui::TableNextRow(ImGuiTableRowFlags_None, 21.0f);
			auto globalAddr = wp.GetGlobalAddr();
			// isActive
			ImGui::TableNextColumn();

			auto isActive = wp.IsActive();
			ImGui::Checkbox(std::format("##{:05X}", globalAddr).c_str(), &isActive);
			if (isActive != wp.IsActive())
			{
				m_debugger.AddWatchpoint(wp.GetId(), wp.GetAccess(), globalAddr,
					wp.GetCondition(), wp.GetValue(), wp.GetType(),
					wp.GetLen(), isActive, wp.GetComment());
				m_reqHexViewer.type = ReqHexViewer::Type::INIT_UPDATE;
				m_reqHexViewer.globalAddr = globalAddr;
				m_reqHexViewer.len = wp.GetLen();
			}
			// GlobalAddr
			ImGui::TableNextColumn();
			ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
			const bool isSelected = selectedAddr == (int)globalAddr;
			if (ImGui::Selectable(std::format("0x{:05X}", globalAddr).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
			{
				selectedAddr = globalAddr;
				m_reqHexViewer.type = ReqHexViewer::Type::INIT_UPDATE;
				m_reqHexViewer.globalAddr = selectedAddr;
				m_reqHexViewer.len = wp.GetLen();
			}
			ImVec2 rowMin = ImGui::GetItemRectMin();
			CheckIfItemClicked(rowMin, showItemContextMenu, id, editedWatchpointId, reqPopup);
			ImGui::PopStyleColor();

			// Access
			DrawProperty(wp.GetAccessS());
			CheckIfItemClicked(rowMin, showItemContextMenu, id, editedWatchpointId, reqPopup);

			// Condition
			std::string condS = wp.GetConditionS();
			if (wp.GetCondition() != Watchpoint::Condition::ANY) {
				if (wp.GetType() == Watchpoint::Type::LEN) 
				{
					condS += std::format("{:02X}", wp.GetValue());
				}
				else {
					condS += std::format("{:04X}", wp.GetValue());
				}
			}
			condS = std::format("{} l:{}", condS, wp.GetLen());
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
				for (const auto& [id, wp] : watchpoints) {
					m_debugger.AddWatchpoint(wp.GetId(), wp.GetAccess(), wp.GetGlobalAddr(),
						wp.GetCondition(), wp.GetValue(), wp.GetType(), wp.GetLen(),
						false, wp.GetComment());
				}
			}
			else if (ImGui::MenuItem("Delete All")) {
				m_debugger.DelWatchpoints();
				m_reqHexViewer.type = ReqHexViewer::Type::NONE;
			};
			ImGui::EndPopup();
		}

		// the item context menu
		if (showItemContextMenu) ImGui::OpenPopup("WpItemMenu");
		if (ImGui::BeginPopup("WpItemMenu"))
		{
			if (editedWatchpointId >= 0)
			{
				const auto& wp = watchpoints.at(editedWatchpointId);

				if (wp.IsActive()) {
					if (ImGui::MenuItem("Disable")) 
					{
						m_debugger.AddWatchpoint(wp.GetId(), wp.GetAccess(), wp.GetGlobalAddr(),
							wp.GetCondition(), wp.GetValue(), wp.GetType(), wp.GetLen(),
							false, wp.GetComment());

						m_reqHexViewer.type = ReqHexViewer::Type::INIT_UPDATE;
						m_reqHexViewer.globalAddr = wp.GetGlobalAddr();
						m_reqHexViewer.len = wp.GetLen();
					}
				}
				else {
					if (ImGui::MenuItem("Enable")) {
						m_debugger.AddWatchpoint(wp.GetId(), wp.GetAccess(), wp.GetGlobalAddr(),
							wp.GetCondition(), wp.GetValue(), wp.GetType(), wp.GetLen(),
							true, wp.GetComment());
						m_reqHexViewer.type = ReqHexViewer::Type::INIT_UPDATE;
						m_reqHexViewer.globalAddr = wp.GetGlobalAddr();
						m_reqHexViewer.len = wp.GetLen();
					}
				}
				if (ImGui::MenuItem("Delete")) {
					m_debugger.DelWatchpoint(editedWatchpointId);
						m_reqHexViewer.type = ReqHexViewer::Type::NONE;
				}
				else if (ImGui::MenuItem("Edit")) {
					reqPopup = ReqPopup::INIT_EDIT;
				};
			}

			ImGui::EndPopup();
		}

		DrawPopup(reqPopup, watchpoints, editedWatchpointId);

	}
	ImGui::PopStyleVar(2);
}

void dev::WatchpointsWindow::DrawPopup(ReqPopup& _reqPopup, const Debugger::Watchpoints& _wps, int _id)
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
	static std::string conditionS = "ANY";
	static std::string valueS = "";
	static int type = static_cast<int>(Watchpoint::Type::LEN);
	static std::string lenS = "FF";
	static std::string commentS = "";
	static ImVec2 buttonSize = { 65.0f, 25.0f };

	// Init for a new WP
	if (_reqPopup == ReqPopup::INIT_ADD) {
		_reqPopup = ReqPopup::ADD;
		commentS = "";
	}

	// Init for editing WP
	if (_reqPopup == ReqPopup::INIT_EDIT)
	{
		_reqPopup = ReqPopup::EDIT;

		const auto& wp = _wps.at(_id);
		oldId = _id;
		isActive = wp.IsActive();
		globalAddrS = std::format("{:04X}", wp.GetGlobalAddr());
		access = wp.GetAccessI();
		conditionS = wp.GetConditionS();
		type = static_cast<int>(wp.GetType());
		if (type == static_cast<int>(Watchpoint::Type::WORD)) {
			valueS = std::format("{:04X}", wp.GetValue());
		}
		else {
			valueS = std::format("{:02X}", wp.GetValue());
		}
		
		lenS = std::format("{:04X}", wp.GetLen());
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
			// status
			DrawProperty2EditableCheckBox("Active", "##WpContextStatus", &isActive);
			// addr
			DrawProperty2EditableS("Global Address", "##WpContextAddress", &globalAddrS, "0x100",
				"A hexademical address in the format 0x100 or 100");
			// access
			DrawProperty2Access("Access", &access, "R - read, W - write, RW - read or write");
			// condition
			DrawProperty2EditableS("Condition", "##WpContextCondition", &conditionS, "",
				"Leave it empty to catch every change.\n"
				"= to break when it's equal to a value.\n"
				"> to break when it's bigger than a value.\n"
				"also works <, >=, <=, !=, ==\n"
			);
			auto cond = Watchpoint::StrToCondition(conditionS);

			// value
			if (cond == Watchpoint::Condition::ANY) ImGui::BeginDisabled();
			DrawProperty2EditableS("Value", "##WpContextValue", &valueS, 
				type == static_cast<int>(Watchpoint::Type::LEN) ? "FF" : "FFFF",
				"A hexademical value in the format 0x100 or 100");
			if (cond == Watchpoint::Condition::ANY) ImGui::EndDisabled();

			// type
			DrawProperty2Type("Type", &type);
			
			// len
			if (type == static_cast<int>(Watchpoint::Type::WORD)) {
				ImGui::BeginDisabled();
				lenS = "2";
			}
			DrawProperty2EditableS("Length", "##WpContextLen", &lenS, "FFFF", 
				"A hexademical value in the format 100", 
				ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
			if (type == static_cast<int>(Watchpoint::Type::WORD)) ImGui::EndDisabled();

			// comment
			DrawProperty2EditableS("Comment", "##WpContextComment", &commentS, "");

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			// warnings
			GlobalAddr globalAddr = dev::StrHexToInt(globalAddrS.c_str());
			std::string warningS = "";
			if (globalAddr > Memory::GLOBAL_MEMORY_LEN - 1) {
				warningS = "Too large address";
			}
			if (cond == Watchpoint::Condition::INVALID) {
				warningS = "Unsupported condition";
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
				GlobalAddr value = dev::StrHexToInt(valueS.c_str());

				m_debugger.AddWatchpoint(id, static_cast<Watchpoint::Access>(access), globalAddr,
					cond, value, static_cast<Watchpoint::Type>(type),
					len, isActive, commentS);

				m_reqHexViewer.type = ReqHexViewer::Type::INIT_UPDATE;
				m_reqHexViewer.globalAddr = globalAddr;
				m_reqHexViewer.len = len;
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