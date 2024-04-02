#include <format>
#include "WatchpointsWindow.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/StringUtils.h"

dev::WatchpointsWindow::WatchpointsWindow(Debugger& _debugger,
	const float* const _fontSizeP, const float* const _dpiScaleP, bool& _reqDisasmUpdate)
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

	ImGui::RadioButton("R ##wpAccessR", _access, 0); ImGui::SameLine();
	ImGui::RadioButton("W ##wpAccessW", _access, 1); ImGui::SameLine();
	ImGui::RadioButton("RW ##wpAccessRW", _access, 2);

	if (*_hint != '\0') {
		ImGui::SameLine();
		dev::DrawHelpMarker(_hint);
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
	
	ImGui::RadioButton("byte ##wpSizeB", _size, 0); ImGui::SameLine();
	ImGui::RadioButton("word ##wpSizeW", _size, 1);

	if (*_hint != '\0') {
		ImGui::SameLine();
		dev::DrawHelpMarker(_hint);
	}
}

// should be called right after ImGui::EndTable();
void dev::WatchpointsWindow::DrawContextMenu(const char* _itemID)
{
	static bool isActive = true;
	static std::string globalAddrS = "0x100";
	static int access = static_cast<int>(Watchpoint::Access::RW);
	static std::string conditionS = "ANY";
	static std::string valueS = "";
	static int size = 0;
	static std::string commentS = "";
	static ImVec2 buttonSize = { 65.0f, 25.0f };

	if (ImGui::BeginPopupContextItem(_itemID))
	{
		static ImGuiTableFlags flags =
			ImGuiTableFlags_ScrollY |
			ImGuiTableFlags_SizingStretchSame |
			ImGuiTableFlags_ContextMenuInBody;

		if (ImGui::BeginTable("##BPContextMenu", 2, flags))
		{
			ImGui::TableSetupColumn("##BPContextMenuName", ImGuiTableColumnFlags_WidthFixed, 200);
			ImGui::TableSetupColumn("##BPContextMenuName", ImGuiTableColumnFlags_WidthFixed, 200);
			// status
			DrawProperty2EditableCheckBox("Active", "##BPContextStatus", &isActive);
			// addr
			DrawProperty2EditableS("Global Address", "##BPContextAddress", &globalAddrS, "0x100");
			// access
			DrawProperty2Access("Access", &access, "R - read, W - write, RW - read and write\n");
			// condition
			DrawProperty2EditableS("Condition", "##BPContextCondition", &conditionS, "",
				"Leave it empty to catch every change.\n"
				"= to break when it's equal to a value.\n"
				"> to break when it's bigger than a value.\n"
				"also works <, >=, <=, !=, ==\n"
			);
			// value
			DrawProperty2EditableS("Value", "##BPContextValue", &valueS);
			// size
			DrawProperty2Size("Size", &size);
			// comment
			DrawProperty2EditableS("Comment", "##BPContextComment", &commentS, "");

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
				m_debugger.AddWatchpoint(-1, static_cast<Watchpoint::Access>(access), globalAddr,
					cond, value,
					size + 1, isActive, commentS);

				m_reqDisasmUpdate = true;
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

void dev::WatchpointsWindow::DrawTable()
{
	static int selectedAddr = 0;
	const char* tableName = "##Watchpoints";
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 0.0f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });

	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Hideable;
	if (ImGui::BeginTable(tableName, 5, flags))
	{
			ImGui::TableSetupColumn("##WPActive", ImGuiTableColumnFlags_WidthFixed, 25);
			ImGui::TableSetupColumn("GlobalAddr", ImGuiTableColumnFlags_WidthFixed, 110);
			ImGui::TableSetupColumn("Access", ImGuiTableColumnFlags_WidthFixed, 50);
			ImGui::TableSetupColumn("Condition", ImGuiTableColumnFlags_WidthFixed, 110);
			ImGui::TableSetupColumn("Comment", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();

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
					m_reqDisasmUpdate = true;
				}
				// GlobalAddr
				ImGui::TableNextColumn();
				ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
				const bool isSelected = selectedAddr == (int)globalAddr;
				if (ImGui::Selectable(std::format("0x{:05X}", globalAddr).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
				{
					selectedAddr = globalAddr;
				}
				ImGui::PopStyleColor();

				// Access
				DrawProperty(wp.GetAccessS());
				// Condition
				std::string condS = wp.GetConditionS();
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
		}

		PopStyleCompact();
		ImGui::EndTable();
	}
	DrawContextMenu(tableName);
	ImGui::PopStyleVar(2);
}