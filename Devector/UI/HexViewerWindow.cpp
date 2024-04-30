#include "HexViewerWindow.h"

#include "Utils/ImGuiUtils.h"
#include "imgui.h"

dev::HexViewerWindow::HexViewerWindow(Hardware& _hardware, Debugger& _debugger,
		const float* const _fontSizeP, const float* const _dpiScaleP, ReqHexViewer& _reqHexViewer)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_hardware(_hardware), m_debugger(_debugger), m_ram(), m_reqHexViewer(_reqHexViewer)
{}

void dev::HexViewerWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Memory Viewer", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar);

	bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");
	UpdateData(isRunning);

	DrawHex(isRunning);

	ImGui::End();
}

void dev::HexViewerWindow::UpdateData(const bool _isRunning)
{
	if (_isRunning) return;

	// check if the hardware updated its state
	auto res = m_hardware.Request(Hardware::Req::GET_REGS);
	const auto& data = *res;

	uint64_t cc = data["cc"];
	auto ccDiff = cc - m_ccLast;
	if (ccDiff == 0) return;
	m_ccLast = cc;

	// update
	auto memP = m_hardware.GetRam()->data();
	std::copy(memP, memP + Memory::MEMORY_MAIN_LEN, m_ram.begin() + m_memPageIdx * Memory::MEM_64K);
	m_debugger.UpdateLastRW();
}

enum class Element : int { MAIN_RAM = 0, RAM_DISK_B0, RAM_DISK_B1, RAM_DISK_B2, RAM_DISK_B3, COUNT };
static const char* elems_names[static_cast<int>(Element::COUNT)] = { "Main Ram", "Ram-Disk0 Bank0", "Ram-Disk0 Bank1", "Ram-Disk0 Bank2", "Ram-Disk0 Bank3" };

void dev::HexViewerWindow::DrawHex(const bool _isRunning)
{
	// memory page selection
	m_memPageIdx = dev::Max(0, m_memPageIdx);
	m_memPageIdx = dev::Min(static_cast<int>(Element::COUNT)-1, m_memPageIdx);
	const char* elem_name = elems_names[m_memPageIdx];
	if (ImGui::SliderInt("##pageSelection", &m_memPageIdx, 0, static_cast<int>(Element::COUNT) - 1, elem_name, ImGuiSliderFlags_NoInput))
	{
		// update
		auto memP = m_hardware.GetRam()->data();
		auto pageOffset = m_memPageIdx * Memory::MEM_64K;
		std::copy(memP + pageOffset, memP + pageOffset + Memory::MEMORY_MAIN_LEN, m_ram.begin());
	}

	// select the highlight mode (RW/R/W)
	static int highlightMode = 0; // 0 - RW, 1 - R, 2 - W
	ImGui::Text("Highlight: "); ImGui::SameLine();
	if (ImGui::RadioButton("RW", &highlightMode, 0)); ImGui::SameLine();
	if (ImGui::RadioButton("R", &highlightMode, 1)); ImGui::SameLine();
	if (ImGui::RadioButton("W", &highlightMode, 2)); ImGui::SameLine();
	dev::DrawHelpMarker(
		"Blue highlights represent reads.\n"
		"Red highlights represent writes.\n"
		"The brighter the color, the more recent the change.");

	constexpr auto headerColumn = "00\0 01\0 02\0 03\0 04\0 05\0 06\0 07\0 08\0 09\0 0A\0 0B\0 0C\0 0D\0 0E\0 0F\0";
	
	const int COLUMNS_COUNT = 17;
	const char* tableName = "##HexViewer";

	static ImGuiTableFlags flags =
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_HighlightHoveredColumn |
		ImGuiTableFlags_BordersOuter | 
		ImGuiTableFlags_Hideable;
	if (ImGui::BeginTable(tableName, COLUMNS_COUNT, flags))
	{
		ImGui::TableSetupScrollFreeze(0, 1);

		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 40);
		for (int column = 0; column < COLUMNS_COUNT - 1; column++)
		{
				ImGui::TableSetupColumn(headerColumn + column*4, ImGuiTableColumnFlags_WidthFixed, 18);
		}
		
		ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
		for (int column = 0; column < COLUMNS_COUNT; column++)
		{
			ImGui::TableSetColumnIndex(column);
			const char* column_name = ImGui::TableGetColumnName(column); // Retrieve the name passed to TableSetupColumn()
			if (column == 0) {
				dev::DrawHelpMarker(
					"Shows the current values of the memory data.\n\n"

					"Red highligh indicates recently written data\n"
					"Blue highligh indicates recently read data\n");
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ADDR);
				ImGui::TableHeader(column_name);
				ImGui::PopStyleColor();
			}
		}

		// Set the scroll position to the selected watchpoint addr
		if (m_reqHexViewer.type == ReqHexViewer::Type::INIT_UPDATE)
		{
			m_reqHexViewer.type = ReqHexViewer::Type::UPDATE;
			float cellPaddingY = ImGui::GetStyle().CellPadding.y;
			float offset = 2.0f;
			Addr addr = m_reqHexViewer.globalAddr & 0xFFFF;
			m_memPageIdx = m_reqHexViewer.globalAddr >> 16;
			ImGui::SetScrollY((addr >> 4) * (*m_fontSizeP + cellPaddingY + offset) * (*m_dpiScaleP));
		}

		// addr & data
		int idx = 0;
		static int addrHovered = -1;
		ImU32 bgColorHeadrHovered = BG_COLOR_ADDR;
		ImGuiListClipper clipper;
		clipper.Begin(int(m_ram.size()) / (COLUMNS_COUNT - 1));
		while (clipper.Step())
		{
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
			{
				ImGui::TableNextRow();
				
				// addr. left header 
				ImGui::TableNextColumn();
				Addr headerAddr = row * (COLUMNS_COUNT - 1);
				if ((addrHovered & 0xFFFF0) == headerAddr){
					bgColorHeadrHovered = BG_COLOR_ADDR_HOVER;
					addrHovered = -1; // reset the highlighting
				}
				else {
					bgColorHeadrHovered = BG_COLOR_ADDR;
				}
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, bgColorHeadrHovered);
				ImGui::TextColored(COLOR_ADDR, std::format("{:04X}", row * (COLUMNS_COUNT - 1)).c_str());

				// the row of data
				for (int col = 0; col < 16; col++)
				{
					Addr addr = row * (COLUMNS_COUNT - 1) + col;
					ImGui::TableNextColumn();

					// calc the cell pos & size
					float offsetX = 4;
					float offsetY = 2;
					ImVec2 highlightPos = ImGui::GetCursorScreenPos();
					highlightPos.x -= offsetX;
					highlightPos.y -= offsetY;
					ImVec2 textSize = ImGui::CalcTextSize("FF");
					ImVec2 highlightEnd = ImVec2(highlightPos.x + textSize.x + offsetX * 2 + 1, highlightPos.y + textSize.y + offsetY * 2);

					// highlight a selected watchpoint
					if (m_reqHexViewer.type != ReqHexViewer::Type::NONE && 
						addr >= m_reqHexViewer.globalAddr && addr < m_reqHexViewer.len + m_reqHexViewer.globalAddr){
						ImGui::GetWindowDrawList()->AddRectFilled(highlightPos, highlightEnd, IM_COL32(100, 100, 100, 255));
					}

					if (!_isRunning) {
						int lastRWIdx = m_debugger.GetLastRW()->at(addr + m_memPageIdx * Memory::MEM_64K);
						auto lastReadsIdx = lastRWIdx & 0xFFFF;
						auto lastWritesIdx = lastRWIdx >> 16;
						
						// highlight the last reads
						if ((highlightMode == 0 || highlightMode == 1) && lastReadsIdx > 0)
						{
							ImU32 color = IM_COL32(
								20 * lastReadsIdx / Debugger::LAST_RW_MAX,
								20 * lastReadsIdx / Debugger::LAST_RW_MAX,
								255 * lastReadsIdx / Debugger::LAST_RW_MAX, 150);

							ImGui::GetWindowDrawList()->AddRectFilled(highlightPos, highlightEnd, color);
						}
						// highlight the last writes
						if ((highlightMode == 0 || highlightMode == 2 ) && lastWritesIdx > 0)
						{
							ImU32 color = IM_COL32(
								255 * lastWritesIdx / Debugger::LAST_RW_MAX,
								20 * lastWritesIdx / Debugger::LAST_RW_MAX,
								20 * lastWritesIdx / Debugger::LAST_RW_MAX, 150);
							ImGui::GetWindowDrawList()->AddRectFilled(highlightPos, highlightEnd, color);
						}
					}

					// highlight the hovered byte
					if (ImGui::IsMouseHoveringRect(highlightPos, highlightEnd) &&
						!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
					{
						ImGui::GetWindowDrawList()->AddRectFilled(highlightPos, highlightEnd, BG_COLOR_BYTE_HOVER);
						ImGui::BeginTooltip();
						ImGui::Text("Address: 0x%04X\n", addr);
						ImGui::EndTooltip();
						addrHovered = addr;
					}
					if (_isRunning) ImGui::BeginDisabled();
					ImGui::TextColored(COLOR_VALUE, std::format("{:02X}", m_ram[addr]).c_str());
					if (_isRunning) ImGui::EndDisabled();

					idx++;
				}
			}
		}
		ImGui::EndTable();
	}
}
