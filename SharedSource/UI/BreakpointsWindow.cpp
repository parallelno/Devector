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

void dev::BreakpointsWindow::DrawTable()
{
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 0.0f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });

	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Hideable |
		ImGuiTableFlags_ContextMenuInBody;
	if (ImGui::BeginTable("##Breakpoints", 4, flags))
	{
		ImGui::TableSetupColumn("##BPStatus", ImGuiTableColumnFlags_WidthFixed, 30);
		ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 90);
		ImGui::TableSetupColumn("Condition", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		PushStyleCompact(1.0f, 0.0f);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		//DrawRegs();
		ImGui::TableNextColumn();
		//DrawFlags();
		ImGui::TableNextColumn();
		//DrawStack();
		ImGui::TableNextColumn();
		//DrawHardware();

		PopStyleCompact();

		ImGui::EndTable();
	}
	ImGui::PopStyleVar(2);
}