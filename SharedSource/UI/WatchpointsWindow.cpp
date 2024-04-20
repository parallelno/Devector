#include <format>
#include "WatchpointsWindow.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/StringUtils.h"

dev::WatchpointsWindow::WatchpointsWindow(Debugger& _debugger,
	const float* const _fontSizeP, const float* const _dpiScaleP, int& _reqDisasmUpdate)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_debugger(_debugger),
	m_reqDisasmUpdate(_reqDisasmUpdate)
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

void dev::WatchpointsWindow::DrawProperty2Size(
	const char* _name, int* _size, const char* _hint)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	
	ImGui::RadioButton("byte ###WpSizeB", _size, 0); ImGui::SameLine();
	ImGui::RadioButton("word ###WpSizeW", _size, 1);

	if (*_hint != '\0') {
		ImGui::SameLine();
		DrawHelpMarker(_hint);
	}
}

void dev::WatchpointsWindow::DrawTable()
{
	static int selectedAddr = 0;
	bool showItemContextMenu = false;
	static int editedWatchpointId = -1;
	bool showItemEditPopup = false;
	bool showAddNewPopup = false;
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
			const char* column_name = ImGui::TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
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
					wp.GetCondition(), wp.GetValue(),
					wp.GetSize(), isActive, wp.GetComment());
				m_reqDisasmUpdate = REQ_DISASM_DRAW;
			}
			// GlobalAddr
			ImGui::TableNextColumn();
			ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
			const bool isSelected = selectedAddr == (int)globalAddr;
			if (ImGui::Selectable(std::format("0x{:05X}", globalAddr).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
			{
				selectedAddr = globalAddr;
			}
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				showItemContextMenu = true;
				editedWatchpointId = id;
			}
			ImGui::PopStyleColor();

			// Access
			DrawProperty(wp.GetAccessS());

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				showItemContextMenu = true;
				editedWatchpointId = id;
			}

			// Condition
			std::string condS = wp.GetConditionS();

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				showItemContextMenu = true;
				editedWatchpointId = id;
			}

			// Value
			if (wp.GetCondition() != Watchpoint::Condition::ANY) {
				if (wp.GetSize() == Watchpoint::VAL_WORD_SIZE) {
					condS += std::format(" {:02X}", wp.GetValue());
				}
				else {
					condS += std::format(" {:04X}", wp.GetValue());
				}
			}
			DrawProperty(condS + wp.GetSizeS());

			// Comment
			DrawProperty(wp.GetComment());

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				showItemContextMenu = true;
				editedWatchpointId = id;
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
			showAddNewPopup = true;
		}

		// the context menu
		if (ImGui::BeginPopupContextItem("WpContextMenu",
			ImGuiPopupFlags_NoOpenOverItems |
			ImGuiPopupFlags_NoOpenOverExistingPopup |
			ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Add New")) {
				showAddNewPopup = true;
			}
			else if (ImGui::MenuItem("Disable All"))
			{
				for (const auto& [id, wp] : watchpoints) {
					m_debugger.AddWatchpoint(wp.GetId(), wp.GetAccess(), wp.GetGlobalAddr(),
						wp.GetCondition(), wp.GetValue(),
						wp.GetSize(), false, wp.GetComment());
				}
				m_reqDisasmUpdate = REQ_DISASM_DRAW;
			}
			else if (ImGui::MenuItem("Delete All")) {
				m_debugger.DelWatchpoints();
				m_reqDisasmUpdate = REQ_DISASM_DRAW;
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
					if (ImGui::MenuItem("Disable")) {
						m_debugger.AddWatchpoint(wp.GetId(), wp.GetAccess(), wp.GetGlobalAddr(),
							wp.GetCondition(), wp.GetValue(),
							wp.GetSize(), false, wp.GetComment());
						m_reqDisasmUpdate = REQ_DISASM_DRAW;
					}
				}
				else {
					if (ImGui::MenuItem("Enable")) {
						m_debugger.AddWatchpoint(wp.GetId(), wp.GetAccess(), wp.GetGlobalAddr(),
							wp.GetCondition(), wp.GetValue(),
							wp.GetSize(), true, wp.GetComment());
						m_reqDisasmUpdate = REQ_DISASM_DRAW;
					}
				}
				if (ImGui::MenuItem("Delete")) {
					m_debugger.DelWatchpoint(editedWatchpointId);
					m_reqDisasmUpdate = REQ_DISASM_DRAW;
				}
				else if (ImGui::MenuItem("Edit")) {
					showItemEditPopup = REQ_DISASM_DRAW;
				};
			}

			ImGui::EndPopup();
		}

		if (showItemEditPopup || showAddNewPopup) ImGui::OpenPopup("WpEdit");
		DrawPopupEdit(showAddNewPopup, showItemEditPopup, watchpoints, editedWatchpointId);

	}
	ImGui::PopStyleVar(2);
}

void dev::WatchpointsWindow::DrawPopupEdit(const bool _addNew, const bool _init, const Debugger::Watchpoints& _wps, int _id)
{
	static bool isActive = true;
	static int oldId = -1;
	static std::string globalAddrS = "0x100";
	static int access = static_cast<int>(Watchpoint::Access::RW);
	static std::string conditionS = "ANY";
	static std::string valueS = "";
	static int size = 0;
	static std::string commentS = "";
	static ImVec2 buttonSize = { 65.0f, 25.0f };

	if (!_addNew && _init && _id >= 0)
	{
		const auto& wp = _wps.at(_id);
		oldId = _id;
		isActive = wp.IsActive();
		globalAddrS = std::format("0x{:04X}", wp.GetGlobalAddr());
		access = wp.GetAccessI();
		conditionS = wp.GetConditionS();
		valueS = std::format("0x{:04X}", wp.GetValue());
		size = static_cast<int>(wp.GetSize());
		commentS = wp.GetComment();
	}

	if (ImGui::BeginPopup("WpEdit"))
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
			// value
			DrawProperty2EditableS("Value", "##WpContextValue", &valueS, "0xFF",
				"A hexademical value in the format 0x100 or 100");
			// size
			DrawProperty2Size("Size", &size);
			// comment
			DrawProperty2EditableS("Comment", "##WpContextComment", &commentS, "");

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			// warnings
			bool warning = false;
			GlobalAddr globalAddr = dev::StrHexToInt(globalAddrS.c_str());
			std::string warningS = "";
			if (globalAddr > Memory::GLOBAL_MEMORY_LEN - 1) {
				warningS = "Too large address";
				warning = true;
			}

			auto cond = Watchpoint::StrToCondition(conditionS);
			if (cond == Watchpoint::Condition::INVALID) {
				warningS = "Unsupported condition";
				warning = true;
			}
			GlobalAddr value = dev::StrHexToInt(valueS.c_str());
			if (globalAddr > Memory::GLOBAL_MEMORY_LEN - 1) {
				warningS = "Too large address";
				warning = true;
			}
			ImGui::TextColored(COLOR_WARNING, warningS.c_str());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();

			if (warning) {
				ImGui::BeginDisabled();
			}
			// OK button
			if (ImGui::Button("Ok", buttonSize) && !warning)
			{
				int id = _addNew ? -1 : oldId;

				m_debugger.AddWatchpoint(id, static_cast<Watchpoint::Access>(access), globalAddr,
					cond, value,
					size + 1, isActive, commentS);

				m_reqDisasmUpdate = REQ_DISASM_DRAW;
				ImGui::CloseCurrentPopup();
			}
			if (warning) {
				ImGui::EndDisabled();
			}

			// Cancel button
			ImGui::SameLine(); ImGui::Text(" "); ImGui::SameLine();
			if (ImGui::Button("Cancel", buttonSize)) ImGui::CloseCurrentPopup();

			ImGui::EndTable();
		}

		ImGui::EndPopup();
	}
}