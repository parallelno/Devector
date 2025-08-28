#include "ui/trace_log_window.h"

#include "utils/utils.h"

dev::TraceLogWindow::TraceLogWindow(
	Hardware& _hardware, Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool* _visibleP)
	:
	BaseWindow("Trace Log", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP,
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_HorizontalScrollbar),
	m_hardware(_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::BREAK |
			dev::Signals::TRACE_LOG_DATA_UPDATE,
			std::bind(&dev::TraceLogWindow::CallbackUpdateData,
				this, std::placeholders::_1, std::placeholders::_2),
			m_visibleP));
}


void dev::TraceLogWindow::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	bool isRunning = dev::Signals::HW_RUNNING & _signals;

	if (isRunning) ImGui::TextUnformatted("Break execution");

	DrawLogSave(isRunning);
	DrawFilter(isRunning);
	DrawTable(isRunning);
}


void dev::TraceLogWindow::DrawLogSave(const bool _isRunning)
{
	if (_isRunning) ImGui::BeginDisabled();

	if (ImGui::Checkbox("Save Log", &m_saveLog))
	{
		if (m_saveLog)
		{
			Addr regPC = m_hardware.Request(Hardware::Req::DEBUG_TRACE_LOG_ENABLE);
		}
		else{
			Addr regPC = m_hardware.Request(Hardware::Req::DEBUG_TRACE_LOG_DISABLE);
		}
	}
	if (_isRunning) ImGui::EndDisabled();

	ImGui::SameLine();
	dev::DrawHelpMarker(
		"Saves a trace log to a file in the Devector folder when \n"
		"the emulation is running. This option is very IO intensive\n"
		"and might slow down the emulation.");
}

const char* filterNames[] = {
	 "c*", "+ call",
	 "+ j*", "+ jmp",
	 "+ r*", "+ ret",
	 "+ pchl",
	 "+ rst",
	 "all" };

void dev::TraceLogWindow::DrawFilter(const bool _isRunning)
{
	const char* filterName = filterNames[m_disasmFilter];

	if (_isRunning) ImGui::BeginDisabled();

	if (ImGui::BeginCombo("##filterNames", filterName))
	{
		for (int n = 0; n < IM_ARRAYSIZE(filterNames); n++)
		{
			const bool is_selected = (m_disasmFilter == n);
			if (ImGui::Selectable(filterNames[n], is_selected)) {
				m_disasmFilter = n;

				m_scheduler.AddSignal({dev::Signals::TRACE_LOG_DATA_UPDATE});
			}

			// Set the initial focus when opening the combo
			// (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	if (_isRunning) ImGui::EndDisabled();

	ImGui::SameLine();
		dev::DrawHelpMarker(
			"Select the type of instructions to display."
			"\n\n"
			"\"c*\" displays all conditional call instructions."
			"\n\n"
			"\"+ call\" displays all above + call instructions."
			"\n\n"
			"etc..."
			"\n\n"
			"\"all\" displays all instructions."
		);
}


void dev::TraceLogWindow::DrawTable(const bool _isRunning)
{
	if (!m_traceLogP) return;

	const int COLUMNS_COUNT = 4;
	const char* tableName = "##TLTable";
	int hoveredLineIdx = -1;
	ImVec2 selectionMin = ImGui::GetCursorScreenPos();
	ImVec2 selectionMax = ImVec2(
		selectionMin.x + ImGui::GetWindowWidth(),
		selectionMin.y);

	if (_isRunning) ImGui::BeginDisabled();

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5, 0 });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	static ImGuiTableFlags flags =
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_NoBordersInBodyUntilResize |
		ImGuiTableFlags_Resizable;
	if (ImGui::BeginTable(tableName, COLUMNS_COUNT, flags))
	{
		auto scale = ImGui::GetWindowDpiScale();

		ImGui::TableSetupColumn("",
			ImGuiTableColumnFlags_WidthFixed |
			ImGuiTableColumnFlags_NoResize, 0);
		ImGui::TableSetupColumn("Addr",
			ImGuiTableColumnFlags_WidthFixed |
			ImGuiTableColumnFlags_NoResize, ADDR_W * scale);
		ImGui::TableSetupColumn("command",
			ImGuiTableColumnFlags_WidthFixed, CODE_W * scale);
		ImGui::TableSetupColumn("consts");

		ImGuiListClipper clipper;
		clipper.Begin(int(m_disasmLinesLen));

		while (clipper.Step())
		{
			for (int lineIdx = clipper.DisplayStart;
				lineIdx < clipper.DisplayEnd;
				lineIdx++)
			{
				ImGui::TableNextRow();

				// the line selection/highlight
				ImGui::TableNextColumn();
				ImGui::TableSetBgColor(
					ImGuiTableBgTarget_CellBg, DIS_BG_CLR_BRK);
				const bool isSelected = m_selectedLineIdx == lineIdx;

				if (ImGui::Selectable(
					std::format("##TLLineId{:04d}", lineIdx).c_str(),
					isSelected, ImGuiSelectableFlags_SpanAllColumns))
				{
					m_selectedLineIdx = lineIdx;
				}
				if (!_isRunning)
				{
					selectionMin.y = ImGui::GetItemRectMin().y;
					selectionMax.y = ImGui::GetItemRectMax().y;
					hoveredLineIdx = ImGui::IsMouseHoveringRect(
						selectionMin, selectionMax) ?
						lineIdx :
						hoveredLineIdx;
				}

				const auto& line = m_traceLogP->at(lineIdx);
				int addr = line.addr;

				DrawDisasmAddr(
					_isRunning, line, m_addrHighlight);
				DrawDisasmCode(
					_isRunning, line, m_addrHighlight);
				DrawDisasmConsts(line, MAX_DISASM_LABELS);

			}
		}
		ImGui::EndTable();
	}
	Addr regPC = m_hardware.Request(Hardware::Req::GET_REG_PC)->at("pc");

	ImGui::PopStyleVar(2);

	if (_isRunning) ImGui::EndDisabled();
}


void dev::TraceLogWindow::DrawDisasmAddr(
	const bool _isRunning, const Disasm::Line& _line,
	AddrHighlight& _addrHighlight)
{
	// the addr column
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DASM_BG_CLR_ADDR);

	auto mouseAction = DrawAddr(_isRunning, _line.GetAddrS(),
		DASM_CLR_LABEL_MINOR, dev::IM_VEC4(0xFFFFFFFF),
		_addrHighlight.IsEnabled(_line.addr));

	switch (mouseAction)
	{
	case UIItemMouseAction::LEFT: // Navigate to the address
		m_scheduler.AddSignal(
			{dev::Signals::DISASM_UPDATE, (GlobalAddr)_line.addr});
		break;
	case UIItemMouseAction::RIGHT:
		m_scheduler.AddSignal(
			{dev::Signals::TRACE_LOG_POPUP_OPEN, (GlobalAddr)_line.addr});
		break;
	}
}


void dev::TraceLogWindow::DrawDisasmCode(
	const bool _isRunning, const Disasm::Line& _line,
	AddrHighlight& _addrHighlight)
{
	// draw code
	ImGui::TableNextColumn();
	auto mouseAction = dev::DrawCodeLine(_isRunning, _line, false);

	// when a user did action to the immediate operand
	switch (mouseAction)
	{
	// any case below means that the immediate addr was at least hovered
	case UIItemMouseAction::LEFT: // Navigate to the address
		m_scheduler.AddSignal(
			{dev::Signals::DISASM_UPDATE, (GlobalAddr)_line.imm});
		break;
	// init the immediate value as an addr to let the context menu copy it
	case UIItemMouseAction::RIGHT:
		m_scheduler.AddSignal(
			{dev::Signals::TRACE_LOG_POPUP_OPEN, (GlobalAddr)_line.imm});
		break;
	}
}


void dev::TraceLogWindow::CallbackUpdateData(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	m_traceLogP = m_debugger.GetTraceLog().GetDisasm(
		TraceLog::TRACE_LOG_SIZE, m_disasmFilter);

	m_disasmLinesLen = m_debugger.GetTraceLog().GetDisasmLen();
}
