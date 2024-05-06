#include "TraceLogWindow.h"

#include "Utils/ImGuiUtils.h"
#include "imgui.h"

dev::TraceLogWindow::TraceLogWindow(Hardware& _hardware, Debugger& _debugger,
		const float* const _fontSizeP, const float* const _dpiScaleP, 
		ReqDisasm& _reqDisasm) 
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_hardware(_hardware), m_debugger(_debugger), 
	m_reqDisasm(_reqDisasm),
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
	const int COLUMNS_COUNT = 4;
	const char* tableName = "##TraceLogTable";
	static int selectedLineIdx = 0;
	bool openItemContextMenu = false;
	static int copyToClipboardAddr = -1; // if it's -1, don't add the option, if it's >=0, add the option with the addr = copyToClipboardAddr

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5, 0 });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	static ImGuiTableFlags flags =
		//ImGuiTableFlags_NoPadOuterX |
		ImGuiTableFlags_ScrollY |
		//ImGuiTableFlags_NoClip |
		ImGuiTableFlags_NoBordersInBodyUntilResize |
		ImGuiTableFlags_Resizable;
	if (ImGui::BeginTable(tableName, COLUMNS_COUNT, flags))
	{
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0);
        ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, ADDR_W);
        ImGui::TableSetupColumn("command", ImGuiTableColumnFlags_WidthFixed, CODE_W);
		ImGui::TableSetupColumn("consts");

		// addr & data
		int idx = 0;
		static int addrHovered = -1;
		ImGuiListClipper clipper;
		clipper.Begin(int(m_traceLog.size()));
		while (clipper.Step())
		{
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
			{
				const auto& disasmLine = m_traceLog.at(row);
				ImGui::TableNextRow();
				
				if (_isRunning) ImGui::BeginDisabled();

				int addr = disasmLine.addr;

				// the line selection/highlight
				ImGui::TableNextColumn();
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DISASM_TBL_BG_COLOR_BRK);
				const bool isSelected = selectedLineIdx == row;
				if (ImGui::Selectable(std::format("##TraceLogLineId{:04d}", row).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
				{
					selectedLineIdx = row;
				}

				// the addr column
				ImGui::TableNextColumn();
				DrawAddr(_isRunning, disasmLine, 
					// _onMouseLeft. Navigate to the address
					[&]()
					{
						m_reqDisasm.type = ReqDisasm::Type::NAVIGATE_TO_ADDR;
						m_reqDisasm.addr = addr;
					},
					// _onMouseRight. Add the "Copy to Clipboard" option to the context menu
					[&]()
					{
						copyToClipboardAddr = addr;
						openItemContextMenu = true;
					}
				);

				// the instruction column
				ImGui::TableNextColumn();
				dev::DrawCodeLine(false, _isRunning, disasmLine, 
					// _onMouseLeft. Navigate to the address
					[&](const Addr _addr)
					{
						m_reqDisasm.type = ReqDisasm::Type::NAVIGATE_TO_ADDR;
						m_reqDisasm.addr = _addr;
					},
					// _onMouseRight. Add the "Copy to Clipboard" option to the context menu
					[&](const Addr _addr)
					{
						copyToClipboardAddr = _addr;
						openItemContextMenu = true;
					}
				);

				// the constants column
				ImGui::TableNextColumn();
				ImGui::TextColored(DISASM_TBL_COLOR_LABEL_MINOR, "%s", disasmLine.consts.c_str());
				
				if (_isRunning) ImGui::EndDisabled();
			}
		}
		ImGui::EndTable();
	}
	DrawDisasmContextMenu(openItemContextMenu, copyToClipboardAddr);

	ImGui::PopStyleVar(2);
}


int dev::TraceLogWindow::DrawDisasmContextMenu(const bool _openContextMenu, int _copyToClipboardAddr)
{
	if (_openContextMenu) ImGui::OpenPopup("DisasmItemMenu");

	if (ImGui::BeginPopup("DisasmItemMenu"))
	{
		if (_copyToClipboardAddr >= 0 ){
			if (ImGui::MenuItem("Copy To Clipboard")) {
				dev::CopyToClipboard(std::format("0x{:04X}", _copyToClipboardAddr));
				_copyToClipboardAddr = -1;
			}
		}
		ImGui::EndPopup();
	}

	return _copyToClipboardAddr;
}