#include <format>
#include "BreakpointsWindow.h"
#include "Utils\StringUtils.h"

dev::BreakpointsWindow::BreakpointsWindow(Hardware& _hardware,
	const float const* _fontSizeP, const float const* _dpiScaleP)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_hardware(_hardware)
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
	static std::string ttt = "test";
	static ImVec2 buttonSize = { 65.0f, 25.0f };

	if (ImGui::BeginPopupContextItem(_itemID))
	{
		static ImGuiTableFlags flags =
			ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
			ImGuiTableFlags_SizingStretchSame |
			ImGuiTableFlags_NoPadInnerX |
			ImGuiTableFlags_ContextMenuInBody;

		if (ImGui::BeginTable("##BPContextMenu", 2, flags))
		{
			ImGui::TableSetupColumn("##BPContextMenuName", ImGuiTableColumnFlags_WidthFixed, 180);
			ImGui::TableSetupColumn("##BPContextMenuName", ImGuiTableColumnFlags_WidthFixed, 180);

			DrawProperty2("Status##BPContext", std::format("{:04X}", m_hardware.m_cpu.GetAF()));
			DrawProperty2("Memory Type##BPContext", std::format("{:04X}", m_hardware.m_cpu.GetBC()));
			//DrawProperty2("Address##BPContext", std::format("{:04X}", m_hardware.m_cpu.GetDE()));
			DrawPropertyEditable2("Address##BPContext", &ttt);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			
			ImGui::Text(""); ImGui::Text("\t"); ImGui::SameLine();
			if (ImGui::Button("Ok", buttonSize)) ImGui::CloseCurrentPopup();
			ImGui::SameLine(); ImGui::Text(" "); ImGui::SameLine();
			if (ImGui::Button("Cancel", buttonSize)) ImGui::CloseCurrentPopup();

			ImGui::EndTable();
		}
		/*
		if (ImGui::Button("Ok")) ImGui::CloseCurrentPopup();
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
		*/
		ImGui::EndPopup();
	}
}

void dev::BreakpointsWindow::DrawTable()
{
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 0.0f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });

	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Hideable;
		//ImGuiTableFlags_ContextMenuInBody;
	if (ImGui::BeginTable("##Breakpoints", 3, flags))
	{
		ImGui::TableSetupColumn("##BPActive", ImGuiTableColumnFlags_WidthFixed, 30);
		ImGui::TableSetupColumn("Mem Type", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 90);
		ImGui::TableHeadersRow();

		PushStyleCompact(1.0f, 0.0f);

		auto& breakpoints = m_hardware.m_debugger.GetBreakpoints();

		for (const auto& [addr, breakpoint] : breakpoints)
		{
			ImGui::TableNextRow();

			// isActive
			DrawProperty(breakpoint.IsActiveS());
			DrawProperty("M1---");
			DrawProperty(std::format("0x{:04X}", addr));
		}

		PopStyleCompact();
		/*
		int hovered_column = -1;
		ImGui::PushID(2);
		if (ImGui::TableGetColumnFlags(2) & ImGuiTableColumnFlags_IsHovered)
			hovered_column = 2;
		if (hovered_column == 2 && !ImGui::IsAnyItemHovered() && ImGui::IsMouseReleased(1))
			ImGui::OpenPopup("MyPopup");
		if (ImGui::BeginPopup("MyPopup"))
		{
			if (2 == 3)
				ImGui::Text("This is a custom popup for unused space after the last column.");
			else
				ImGui::Text("This is a custom popup for Column %d", 2);
			if (ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		ImGui::PopID();
		*/

		ImGui::EndTable();
	}
	/*
	if (ImGui::BeginPopupContextItem("##Breakpoints")) // <-- use last item id as popup id
	{
		ImGui::Text("This a popup for \"\"!");
		if (ImGui::Button("Close"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
	*/
	DrawContextMenu("##Breakpoints");

	ImGui::PopStyleVar(2);
}
