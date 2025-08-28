#include "ui/hex_viewer_window.h"
#include "ui/hex_viewer_window_consts.h"


#include "imgui_stdlib.h"

dev::HexViewerWindow::HexViewerWindow(Hardware& _hardware, Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool* _visibleP)
	:
	BaseWindow("Hex Viewer", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP,
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_HorizontalScrollbar),
	m_hardware(_hardware), m_debugger(_debugger), m_ram()
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::HW_RUNNING |
			dev::Signals::BREAK,
			std::bind(&dev::HexViewerWindow::CallbackUpdateData,
				this, std::placeholders::_1, std::placeholders::_2),
			m_visibleP, 1000ms));

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::HEX_VIEWER_DATA_UPDATE,
			std::bind(&dev::HexViewerWindow::CallbackUpdateData,
				this, std::placeholders::_1, std::placeholders::_2),
			m_visibleP));

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::HEX_HIGHLIGHT_ON,
			std::bind(&dev::HexViewerWindow::CallbackHighlightOn,
				this, std::placeholders::_1, std::placeholders::_2),
			m_visibleP));

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::HEX_HIGHLIGHT_OFF,
			std::bind(&dev::HexViewerWindow::CallbackHighlightOff,
				this, std::placeholders::_1, std::placeholders::_2),
			m_visibleP));
}

void dev::HexViewerWindow::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	bool isRunning = dev::Signals::HW_RUNNING & _signals;

	DrawSearchBar();
	DrawMemPageSelector();
	DrawRWMode();
	DrawHexTable(isRunning);
}


void dev::HexViewerWindow::DrawSearchBar()
{
	// draw an addr search
	if (ImGui::InputInt(
		"##addrSelection", &m_searchAddr, 1, 0x10000,
		ImGuiInputTextFlags_CharsHexadecimal |
		ImGuiInputTextFlags_AutoSelectAll))
	{
		m_searchAddr = dev::Max(0, m_searchAddr);
		m_searchAddr = dev::Min(m_searchAddr, Memory::MEMORY_GLOBAL_LEN - 1);

		bool memPageIdxChanged = m_searchAddr >> 16 != m_memPageIdx;
		m_memPageIdx = m_searchAddr >> 16;

		if (memPageIdxChanged){
			m_scheduler.AddSignal({dev::Signals::HEX_VIEWER_DATA_UPDATE});
		}

		m_scheduler.AddSignal(
			{dev::Signals::HEX_HIGHLIGHT_ON,
				Scheduler::GlobalAddrLen{(GlobalAddr)m_searchAddr, 1}});
	}

	ImGui::SameLine();
	dev::DrawHelpMarker("A hexademical value in the format FF");
}


void dev::HexViewerWindow::DrawMemPageSelector()
{
	// memory page selector
	m_memPageIdx = dev::Max(0, m_memPageIdx);
	m_memPageIdx = dev::Min(static_cast<int>(HexViewer::PAGES_MAX)-1, m_memPageIdx);
	const char* elem_name = HexViewer::page_names[m_memPageIdx];
	bool pageSelected = ImGui::SliderInt("##pageSelection",
		&m_memPageIdx, 0, static_cast<int>(HexViewer::PAGES_MAX) - 1,
		elem_name, ImGuiSliderFlags_NoInput);

	if (pageSelected){
		m_scheduler.AddSignal({dev::Signals::HEX_VIEWER_DATA_UPDATE});
	}
}

void dev::HexViewerWindow::DrawRWMode()
{
	// select the highlight mode (RW/R/W)
	ImGui::Text("Highlight: "); ImGui::SameLine();
	ImGui::RadioButton("RW", &m_highlightMode, 0); ImGui::SameLine();
	ImGui::RadioButton("R", &m_highlightMode, 1); ImGui::SameLine();
	ImGui::RadioButton("W", &m_highlightMode, 2); ImGui::SameLine();
	dev::DrawHelpMarker(
		"Blue highlight represents reads.\n"
		"Red highlight represents writes.\n"
		"The brighter the color, the more recent the change.");
}

void dev::HexViewerWindow::DrawHexTable(const bool _isRunning)
{
	// first column for addr,
	// then 16 bytes,
	// then a gap,
	// then 16 chars

	constexpr int DATA_LINE_LEN = 16;
	constexpr int COLUMNS_COUNT = 1 + DATA_LINE_LEN + 1 + DATA_LINE_LEN;
	const char* tableName = "##HexViewer";

	static ImGuiTableFlags flags =
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_HighlightHoveredColumn |
		ImGuiTableFlags_BordersOuter |
		ImGuiTableFlags_Hideable;
	if (ImGui::BeginTable(tableName, COLUMNS_COUNT, flags))
	{
		ImGui::TableSetupScrollFreeze(0, 1);

		auto scale = ImGui::GetWindowDpiScale();

		// Addr
		ImGui::TableSetupColumn("#hexviewe_addr",
			ImGuiTableColumnFlags_WidthFixed, 40 * scale);
		// Data
		for (const char* column_name : HexViewer::col_names1)
		{
			ImGui::TableSetupColumn(column_name,
				ImGuiTableColumnFlags_WidthFixed, 18 * scale);
		}
		// Gap
		ImGui::TableSetupColumn("#hexviewe_gap",
			ImGuiTableColumnFlags_WidthFixed, 4 * scale);
		// Chars
		for (const char* column_name : HexViewer::col_names2)
		{
			ImGui::TableSetupColumn(column_name,
				ImGuiTableColumnFlags_WidthFixed, 6 * scale);
		}

		ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
		for (int column = 0; column < COLUMNS_COUNT; column++)
		{
			ImGui::TableSetColumnIndex(column);
			// Retrieve the name passed to TableSetupColumn()
			const char* column_name = ImGui::TableGetColumnName(column);
			if (column == 0) {
				dev::DrawHelpMarker(
					"Shows the current values of the memory data.\n\n"

					"Red highligh indicates recently written data\n"
					"Blue highligh indicates recently read data\n");
			}
			else if (column >= 1 && column <= 16) {
				ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ADDR);
				ImGui::TableHeader(column_name);
				ImGui::PopStyleColor();
			}
		}


		// scroll to the highlighted addr
		if (m_table_scroll_y >= 0)
		{
			ImGui::SetScrollY(m_table_scroll_y);
			m_table_scroll_y = -1;
		}

		// addr & data
		int idx = 0;
		static int addrHovered = -1;
		ImU32 bgColorHeadrHovered = BG_COLOR_ADDR;
		ImGuiListClipper clipper;

		clipper.Begin(int(m_ram.size()) / (DATA_LINE_LEN));

		while (clipper.Step())
		{
			for (int row = clipper.DisplayStart;
					row < clipper.DisplayEnd; row++)
			{
				ImGui::TableNextRow();

				// addr. left header
				ImGui::TableNextColumn();
				Addr headerAddr = row * DATA_LINE_LEN;
				if ((addrHovered & 0xFFFF0) == headerAddr){
					bgColorHeadrHovered = BG_COLOR_ADDR_HOVER;
					addrHovered = -1; // reset the highlighting
				}
				else {
					bgColorHeadrHovered = BG_COLOR_ADDR;
				}
				ImGui::TableSetBgColor(
					ImGuiTableBgTarget_CellBg,
					bgColorHeadrHovered);

				ImGui::TextColored(COLOR_ADDR,
					std::format("{:04X}", row * DATA_LINE_LEN).c_str());

				// Draw 16 bytes
				for (int col = 0; col < DATA_LINE_LEN; col++)
				{
					Addr addr = row * DATA_LINE_LEN + col;
					ImGui::TableNextColumn();

					// calc the cell pos & size
					float offsetX = 4;
					float offsetY = 2;
					ImVec2 highlightPos = ImGui::GetCursorScreenPos();
					highlightPos.x -= offsetX;
					highlightPos.y -= offsetY;
					ImVec2 textSize = ImGui::CalcTextSize("FF");
					ImVec2 highlightEnd = ImVec2(
						highlightPos.x + textSize.x + offsetX * 2 + 1,
						highlightPos.y + textSize.y + offsetY * 2);

					// highlight a selected watchpoint
					if (m_status == Status::HIGHLIGHT &&
						addr >= m_highlightAddr &&
						addr < m_highlightAddrLen + m_highlightAddr )
					{
						ImGui::GetWindowDrawList()->AddRectFilled(
							highlightPos, highlightEnd,
							IM_COL32(100, 100, 100, 255));
					}

					if (!_isRunning) {
						int lastRWIdx = m_debugger.GetLastRW()->at(
							addr + m_memPageIdx * Memory::MEM_64K);

						auto lastReadsIdx = lastRWIdx & 0xFFFF;
						auto lastWritesIdx = lastRWIdx >> 16;

						// highlight the last reads
						if ((m_highlightMode == 0 || m_highlightMode == 1) &&
							 lastReadsIdx > 0)
						{
							ImU32 color = IM_COL32(
								20 * lastReadsIdx / Debugger::LAST_RW_MAX,
								20 * lastReadsIdx / Debugger::LAST_RW_MAX,
								255 * lastReadsIdx / Debugger::LAST_RW_MAX, 150);

							ImGui::GetWindowDrawList()->AddRectFilled(
								highlightPos, highlightEnd, color);
						}
						// highlight the last writes
						if ((m_highlightMode == 0 || m_highlightMode == 2 ) &&
							lastWritesIdx > 0)
						{
							ImU32 color = IM_COL32(
								255 * lastWritesIdx / Debugger::LAST_RW_MAX,
								20 * lastWritesIdx / Debugger::LAST_RW_MAX,
								20 * lastWritesIdx / Debugger::LAST_RW_MAX, 150);
							ImGui::GetWindowDrawList()->AddRectFilled(
								highlightPos, highlightEnd, color);
						}
					}

					// highlight the hovered byte
					if (ImGui::IsMouseHoveringRect(highlightPos, highlightEnd) &&
						!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
					{
						ImGui::GetWindowDrawList()->AddRectFilled(
							highlightPos, highlightEnd, BG_COLOR_BYTE_HOVER);
						ImGui::BeginTooltip();
						ImGui::Text("Address: 0x%04X, char: %c\n", addr, m_ram[addr]);
						ImGui::EndTooltip();
						addrHovered = addr;
					}
					if (_isRunning) ImGui::BeginDisabled();
					ImGui::TextColored(COLOR_VALUE,
						std::format("{:02X}", m_ram[addr]).c_str());
					if (_isRunning) ImGui::EndDisabled();

					idx++;
				}

				// Draw a gap
				ImGui::TableNextColumn();

				// Draw 16 chars
				for (int col = 0; col < DATA_LINE_LEN; col++)
				{
					ImGui::TableNextColumn();
					if (_isRunning) ImGui::BeginDisabled();

					char c = m_ram[row * DATA_LINE_LEN + col];
					ImGui::TextColored(COLOR_VALUE,
						(std::isprint(c) ? std::format("{:c}", c) : ".")
							.c_str());

						if (_isRunning) ImGui::EndDisabled();
				}
			}
		}
		ImGui::EndTable();
	}
}


void dev::HexViewerWindow::CallbackHighlightOn(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	using GlobalAddrLen = dev::Scheduler::GlobalAddrLen;

	auto addr_len = std::get<GlobalAddrLen>(*_data);

	m_status = Status::HIGHLIGHT;
	m_memPageIdx = addr_len.globalAddr >> 16;
	m_highlightAddr = addr_len.globalAddr & 0xFFFF;
	m_highlightAddrLen = addr_len.len;

	float cellPaddingY = ImGui::GetStyle().CellPadding.y;
	float offset = 2.0f;

	m_table_scroll_y = (m_highlightAddr >> 4) *
		(ImGui::GetFontSize() + cellPaddingY + offset);
}


void dev::HexViewerWindow::CallbackHighlightOff(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	m_status = Status::NONE;
}



void dev::HexViewerWindow::CallbackUpdateData(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	// update
	auto memP = m_hardware.GetRam()->data();
	auto pageOffset = m_memPageIdx * Memory::MEM_64K;
	std::copy(
		memP + pageOffset,
		memP + pageOffset + Memory::MEMORY_MAIN_LEN,
		m_ram.begin());
	m_debugger.UpdateLastRW();
}
