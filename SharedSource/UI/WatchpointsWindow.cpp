#include <format>
#include "WatchpointsWindow.h"
#include "Utils\StringUtils.h"

dev::WatchpointsWindow::WatchpointsWindow(Hardware& _hardware,
	const float const* _fontSizeP, const float const* _dpiScaleP)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_hardware(_hardware)
{}

void dev::WatchpointsWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Watchpoints", &open, ImGuiWindowFlags_NoCollapse);

	DrawTable();

	ImGui::End();
}

void dev::WatchpointsWindow::DrawTable()
{
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 0.0f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });

	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Hideable |
		ImGuiTableFlags_ContextMenuInBody;
	if (ImGui::BeginTable("##Watchpoints", 3, flags))
	{
		ImGui::TableSetupColumn("##WPStatus", ImGuiTableColumnFlags_WidthFixed, 30);
		ImGui::TableSetupColumn("Address##WPS", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("Value##WPS", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		PushStyleCompact(1.0f, 0.0f);

		ImGui::TableNextRow();
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