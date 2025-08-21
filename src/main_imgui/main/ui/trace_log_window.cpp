#include "ui/trace_log_window.h"

dev::TraceLogWindow::TraceLogWindow(Hardware& _hardware, Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool& _visible, const float* const _dpiScaleP)
	:
	BaseWindow("Trace Log", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visible, _dpiScaleP,
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_HorizontalScrollbar),
	m_hardware(_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::BREAK,
			std::bind(&dev::TraceLogWindow::CallbackUpdateData,
				this, std::placeholders::_1, std::placeholders::_2),
			m_visible));
}

void dev::TraceLogWindow::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	bool isRunning = dev::Signals::HW_RUNNING & _signals;
	DrawLog(isRunning);
}

void dev::TraceLogWindow::CallbackUpdateData(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	m_traceLogP = m_debugger.GetTraceLog().GetDisasm(
		TraceLog::TRACE_LOG_SIZE, m_disasmFilter);

	m_disasmLinesLen = m_debugger.GetTraceLog().GetDisasmLen();
}

void dev::TraceLogWindow::DrawDisasmAddr(const bool _isRunning, const Disasm::Line& _line,
	ContextMenu& _contextMenu, AddrHighlight& _addrHighlight)
{
	// the addr column
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DASM_BG_CLR_ADDR);

	auto mouseAction = DrawAddr(_isRunning, _line.GetAddrS(),
		DASM_CLR_LABEL_MINOR, dev::IM_VEC4(0xFFFFFFFF), _addrHighlight.IsEnabled(_line.addr));
	switch (mouseAction)
	{
	case UIItemMouseAction::LEFT: // Navigate to the address
		m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE, (Addr)_line.addr});
		break;
	case UIItemMouseAction::RIGHT:
		_contextMenu.Init(_line.addr, _line.GetAddrS());
		break;
	}
}

void dev::TraceLogWindow::DrawDisasmCode(const bool _isRunning, const Disasm::Line& _line,
	ContextMenu& _contextMenu, AddrHighlight& _addrHighlight)
{
	// draw code
	ImGui::TableNextColumn();
	auto mouseAction = dev::DrawCodeLine(_isRunning, _line, false);
	// when a user did action to the immediate operand
	switch (mouseAction)
	{
	// any case below means that the immediate addr was at least hovered
	case UIItemMouseAction::LEFT: // Navigate to the address
		m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE, (Addr)_line.imm});
		break;
	case UIItemMouseAction::RIGHT: // init the immediate value as an addr to let the context menu copy it
		//_contextMenu.Init(_line.imm, _line.GetImmediateS(), true);
		break;
	}

	//// set the addr highlight when
	//if (mouseAction != UIItemMouseAction::NONE) {
	//	_addrHighlight.Init(_line.imm);
	//}
}

const char* filterNames[] = { "c*", "+ call", "+ j*", "+ jmp", "+ r*", "+ ret", "+ pchl", "+ rst", "all" };

void dev::TraceLogWindow::DrawLog(const bool _isRunning)
{
	if (!m_traceLogP) return;

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
				m_traceLogP = m_debugger.GetTraceLog().GetDisasm(TraceLog::TRACE_LOG_SIZE, m_disasmFilter);
				m_disasmLinesLen = m_debugger.GetTraceLog().GetDisasmLen();
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
	const char* tableName = "##TLTable";
	int hoveredLineIdx = -1;
	ImVec2 selectionMin = ImGui::GetCursorScreenPos();
	ImVec2 selectionMax = ImVec2(selectionMin.x + ImGui::GetWindowWidth(), selectionMin.y);

	if (_isRunning) ImGui::BeginDisabled();

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

		ImGuiListClipper clipper;
		clipper.Begin(int(m_disasmLinesLen));
		while (clipper.Step())
		{
			for (int lineIdx = clipper.DisplayStart; lineIdx < clipper.DisplayEnd; lineIdx++)
			{
				ImGui::TableNextRow();

				// the line selection/highlight
				ImGui::TableNextColumn();
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DIS_BG_CLR_BRK);
				const bool isSelected = m_selectedLineIdx == lineIdx;
				if (ImGui::Selectable(std::format("##TLLineId{:04d}", lineIdx).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
				{
					m_selectedLineIdx = lineIdx;
				}
				if (!_isRunning)
				{
					selectionMin.y = ImGui::GetItemRectMin().y;
					selectionMax.y = ImGui::GetItemRectMax().y;
					hoveredLineIdx = ImGui::IsMouseHoveringRect(selectionMin, selectionMax) ? lineIdx : hoveredLineIdx;
				}

				const auto& line = m_traceLogP->at(lineIdx);
				int addr = line.addr;

				DrawDisasmAddr(
					_isRunning, line, m_contextMenu, m_addrHighlight);
				DrawDisasmCode(
					_isRunning, line, m_contextMenu, m_addrHighlight);
				DrawDisasmConsts(line, MAX_DISASM_LABELS);

			}
		}
		ImGui::EndTable();
	}
	Addr regPC = m_hardware.Request(Hardware::Req::GET_REG_PC)->at("pc");
	DrawContextMenu(regPC, m_contextMenu);

	ImGui::PopStyleVar(2);

	if (_isRunning) ImGui::EndDisabled();
}


void dev::TraceLogWindow::DrawContextMenu(const Addr _regPC, ContextMenu& _contextMenu)
{
	if (_contextMenu.status == ContextMenu::Status::INIT_CONTEXT_MENU) {
		ImGui::OpenPopup(_contextMenu.contextMenuName);
		_contextMenu.status = ContextMenu::Status::NONE;
	}

	if (ImGui::BeginPopup(_contextMenu.contextMenuName))
	{
		if (ImGui::MenuItem("Copy")) {
			dev::CopyToClipboard(_contextMenu.str);
		}

		ImGui::SeparatorText("");
		if (ImGui::MenuItem("Add/Remove Beakpoint"))
		{
			Breakpoint::Status bpStatus =
				static_cast<Breakpoint::Status>(
					m_hardware.Request(
						Hardware::Req::DEBUG_BREAKPOINT_GET_STATUS,
						{ {"addr", m_contextMenu.addr} })->at("status"));

			if (bpStatus == Breakpoint::Status::DELETED) {
				Breakpoint::Data bpData { m_contextMenu.addr };
				m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_ADD, {
					{"data0", bpData.data0 },
					{"data1", bpData.data1 },
					{"data2", bpData.data2 },
					{"comment", ""}
					});
			}
			else {
				m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DEL, { {"addr", m_contextMenu.addr} });
			}
		}
		ImGui::EndPopup();
	}
}