#include "hardwareStatsWindow.h"
#include <format>

dev::HardwareStatsWindow::HardwareStatsWindow()
{}

void dev::HardwareStatsWindow::Update()
{
	static bool open = true;
	ImGui::Begin("Hardware Stats", &open, ImGuiWindowFlags_NoCollapse);

	DrawGlobalStats();

	ImGui::End();
}

void dev::HardwareStatsWindow::DrawGlobalStats()
{

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (!ImGui::CollapsingHeader("Global Stats")) return;

	static ImGuiTableFlags flags =
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter |
		ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody;
	if (ImGui::BeginTable("GlobalStats_table", 2, flags))
	{
		std::string winLossEvenCount = std::format("({:.1f}%%) / ", (double)12);

		std::string avgWinLossRatio = std::format("{:.2f}", (double)17 / (double)18);
		double avgWinRealized = 21 / (double)19;
		double avgLossRealized = 22 / (double)20;
		double avgR = abs(avgWinRealized / avgLossRealized);
		std::string avgRealizedWinLossR = std::format("{:.2f} / {:.2f} ({:.2f}%%)", avgWinRealized, avgLossRealized, avgR);

		dev::UpdatePropertyPrintStat("P&L / Fee");
		
		DrawSelectableText(avgRealizedWinLossR);

		dev::UpdatePropertyPrintStat("Realized / Unrealized");
		ImGui::Text("$%.2f / $%.2f",
			31,
			32);
		dev::UpdatePropertyPrintStat("Average Daily P&L");
		ImGui::Text("$%.2f", 76);
		dev::UpdatePropertyPrintStat("Win/Loss/Even");
		ImGui::Text(winLossEvenCount.c_str());
		dev::UpdatePropertyPrintStat("Average win/loss ratio");
		ImGui::Text(avgWinLossRatio.c_str());
		dev::UpdatePropertyPrintStat("Average realized win/loss R");
		ImGui::Text(avgRealizedWinLossR.c_str());
		dev::UpdatePropertyPrintStat("Trading days");
		ImGui::Text("%.0f days", 87);
		ImGui::EndTable();
	}
}