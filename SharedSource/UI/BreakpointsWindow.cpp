#include <format>
#include "BreakpointsWindow.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/StringUtils.h"

dev::BreakpointsWindow::BreakpointsWindow(Debugger& _debugger,
	const float* const _fontSizeP, const float* const _dpiScaleP, bool& _reqDisasmUpdate)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_debugger(_debugger),
	m_reqDisasmUpdate(_reqDisasmUpdate)
{}

void dev::BreakpointsWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Breakpoints", &open, ImGuiWindowFlags_NoCollapse);

	DrawTable();

	ImGui::End();
}

void dev::BreakpointsWindow::DrawProperty(const std::string& _name, const ImVec2& _aligment)
{
	ImGui::TableNextColumn();
	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name.c_str(), _aligment);
	ImGui::PopStyleColor();
}

// should be called right after ImGui::EndTable();
void dev::BreakpointsWindow::DrawContextMenu(const char* _itemID)
{
	static bool isActive = true;
	static std::string globalAddrS = "0x100";
	static std::string conditionS = "";
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
			ImGui::TableSetupColumn("##BPContextMenuName", ImGuiTableColumnFlags_WidthFixed, 140);
			ImGui::TableSetupColumn("##BPContextMenuName", ImGuiTableColumnFlags_WidthFixed, 140);
			// status
			DrawProperty2EditableCheckBox("Active", "##BPContextStatus", &isActive);
			// addr
			DrawProperty2EditableS("Global Address", "##BPContextAddress", & globalAddrS, "0x100");
			// condition
			DrawProperty2EditableS("Condition", "##BPContextCondition", &conditionS, "");
			// comment
			DrawProperty2EditableS("Comment", "##BPContextComment", &commentS, "");

			
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			// warning
			bool warning = false;
			GlobalAddr globalAddr = dev::StrHexToInt(globalAddrS.c_str());
			std::string warningS = "";
			if (globalAddr > Memory::GLOBAL_MEMORY_LEN - 1) {
				warningS = "Too large address";
				warning = true;
			}
			//ImGui::SameLine(); 
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
				m_debugger.AddBreakpoint(globalAddr, isActive ? Breakpoint::Status::ACTIVE : Breakpoint::Status::DISABLED, commentS);
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

void dev::BreakpointsWindow::DrawTable()
{
	static int selectedAddr = 0;
	const char* tableName = "##Breakpoints";
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 0.0f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });

	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Hideable;
	if (ImGui::BeginTable(tableName, 4, flags))
	{
		ImGui::TableSetupColumn("##BPActive", ImGuiTableColumnFlags_WidthFixed, 25);
		ImGui::TableSetupColumn("GlobalAddr", ImGuiTableColumnFlags_WidthFixed, 110);
		ImGui::TableSetupColumn("Condition", ImGuiTableColumnFlags_WidthFixed, 180);
		ImGui::TableSetupColumn("Comment", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		PushStyleCompact(1.0f, 0.0f);

		auto breakpoints = m_debugger.GetBreakpoints();

		for (const auto& [addr, breakpoint] : breakpoints)
		{
			ImGui::TableNextRow(ImGuiTableRowFlags_None, 21.0f);

			// isActive
			ImGui::TableNextColumn();
			auto isActive = breakpoint.IsActive();
			ImGui::Checkbox(std::format("##{:05X}", addr).c_str(), &isActive);
			if (isActive != breakpoint.IsActive())
			{
				m_debugger.AddBreakpoint(addr, isActive ? Breakpoint::Status::ACTIVE : Breakpoint::Status::DISABLED, breakpoint.GetComment());
				m_reqDisasmUpdate = true;
			}
			// GlobalAddr
			ImGui::TableNextColumn();
			ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
			const bool isSelected = selectedAddr == (int)addr;
			if (ImGui::Selectable(std::format("0x{:05X}", addr).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
			{
				selectedAddr = addr;
			}
			ImGui::PopStyleColor();
			// Condition
			DrawProperty("");
			// Comment
			DrawProperty(breakpoint.GetComment());
		}

		PopStyleCompact();
		ImGui::EndTable();
	}
	DrawContextMenu(tableName);

	ImGui::PopStyleVar(2);
}
