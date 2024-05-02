#include "TraceLogWindow.h"

#include "Utils/ImGuiUtils.h"
#include "imgui.h"

dev::TraceLogWindow::TraceLogWindow(Hardware& _hardware, Debugger& _debugger,
		const float* const _fontSizeP, const float* const _dpiScaleP, ReqHexViewer& _reqHexViewer)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_hardware(_hardware), m_debugger(_debugger), m_reqHexViewer(_reqHexViewer),
	m_traceLog()
{}
void dev::TraceLogWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Trace Log", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar);

	bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");
	UpdateData(isRunning);
	DrawLog(isRunning);

	ImGui::End();
}

void dev::TraceLogWindow::UpdateData(const bool _isRunning)
{
	if (_isRunning) return;

	// check if the hardware updated its state
	auto res = m_hardware.Request(Hardware::Req::GET_REGS);
	const auto& data = *res;

	uint64_t cc = data["cc"];
	auto ccDiff = cc - m_ccLast;
	if (ccDiff == 0) return;
	m_ccLast = cc;

	m_traceLog = m_debugger.GetTraceLog(0, Debugger::TRACE_LOG_SIZE, m_disasmFilter);
}

const char* filterNames[] = { "call", "+ c*", "+ rst", "+ pchl", "+ jmp", "+ j*", "+ ret, r*", "all" };

void dev::TraceLogWindow::DrawLog(const bool _isRunning)
{
	// filter mode
	const char* filterName = filterNames[m_disasmFilter];
	
	if (_isRunning) ImGui::BeginDisabled();
	if (ImGui::BeginCombo("##filterNames", filterName))
	{
		for (int n = 0; n < IM_ARRAYSIZE(filterNames); n++)
		{
			const bool is_selected = (m_disasmFilter == n);
			if (ImGui::Selectable(filterNames[n], is_selected)) {
				m_disasmFilter = n;
				m_traceLog = m_debugger.GetTraceLog(0, Debugger::TRACE_LOG_SIZE, m_disasmFilter);
			}

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	if (_isRunning) ImGui::EndDisabled();

	// disasm table
	const int COLUMNS_COUNT = 2;
	const char* tableName = "##TraceLogTable";

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	static ImGuiTableFlags flags =
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_HighlightHoveredColumn |
		ImGuiTableFlags_BordersOuter | 
		ImGuiTableFlags_Hideable;
	if (ImGui::BeginTable(tableName, COLUMNS_COUNT, flags))
	{
        ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, ADDR_W);
        ImGui::TableSetupColumn("command", ImGuiTableColumnFlags_WidthFixed, CODE_W);

		// addr & data
		int idx = 0;
		static int addrHovered = -1;
		ImGuiListClipper clipper;
		clipper.Begin(int(m_traceLog.size()) / (COLUMNS_COUNT - 1));
		while (clipper.Step())
		{
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
			{
				const auto& disasmLine = m_traceLog.at(row);
				ImGui::TableNextRow();
				
				if (_isRunning) ImGui::BeginDisabled();

				// the addr column
				ImGui::TableNextColumn();
				Addr headerAddr = row * (COLUMNS_COUNT - 1);
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DISASM_TBL_BG_COLOR_ADDR);
				ImGui::TextColored(DISASM_TBL_COLOR_LABEL_MINOR, disasmLine.addrS.c_str());

				// the instruction column
				ImGui::TableNextColumn();
				//ImGui::TextColored(DISASM_TBL_COLOR_MNEMONIC, " %s", disasmLine.str.c_str());

				dev::DrawCodeLine(_isRunning, disasmLine, 
					// _onMouseLeft. Navigate to the address
					[&](const Addr _addr)
					{
						
					},
					// _onMouseRight. Add the "Copy to Clipboard" option to the context menu
					[&](const Addr _addr)
					{
						
					}
				);
				
				if (_isRunning) ImGui::EndDisabled();
			}
		}
		ImGui::EndTable();
	}

	ImGui::PopStyleVar(1);
}
