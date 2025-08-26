#include <format>

#include "ui/disasm_window.h"
#include "utils/str_utils.h"
#include "imgui_stdlib.h"

dev::DisasmWindow::DisasmWindow(
		dev::Hardware& _hardware, Debugger& _debugger, ImFont* fontComment,
		dev::Scheduler& _scheduler,
		bool* _visibleP, const float* const _dpiScale)
	:
	BaseWindow("Disasm", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP, _dpiScale),
	m_hardware(_hardware),
	m_debugger(_debugger),
	m_fontCommentP(fontComment)
{
	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::BREAK |
			dev::Signals::BREAKPOINTS,
			std::bind(&dev::DisasmWindow::CallbackUpdateAtCC,
				this, std::placeholders::_1, std::placeholders::_2),
			m_visibleP, 1000ms));

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::DISASM_UPDATE,
			std::bind(&dev::DisasmWindow::CallbackUpdateAtAddr,
				this, std::placeholders::_1, std::placeholders::_2),
			m_visibleP));
}

void dev::DisasmWindow::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	bool isRunning = dev::Signals::HW_RUNNING & _signals;

	DrawDebugControls(isRunning);
	DrawSearch(isRunning);
	DrawDisasm(isRunning);
}

void dev::DisasmWindow::DrawDebugControls(const bool _isRunning)
{
	if (!_isRunning && ImGui::Button(" Run ")|
		ImGui::IsKeyPressed(ImGuiKey_F5))
	{
		m_hardware.Request(Hardware::Req::RUN);
	}
	else if (_isRunning && ImGui::Button("Break")|
		ImGui::IsKeyPressed(ImGuiKey_F6))
	{
		m_hardware.Request(Hardware::Req::STOP);
	}

	if (_isRunning) ImGui::BeginDisabled();

	 ImGui::SameLine();
	if (ImGui::Button("Step") | ImGui::IsKeyPressed(ImGuiKey_F7))
	{
		m_hardware.Request(Hardware::Req::STOP);
		m_hardware.Request(Hardware::Req::EXECUTE_INSTR);
	}
	ImGui::SameLine();
	if (ImGui::Button("Step Over") | ImGui::IsKeyPressed(ImGuiKey_F8))
	{
		Addr addr = m_hardware.Request(
			Hardware::Req::GET_STEP_OVER_ADDR)->at("data");
		Breakpoint::Data bpData{
			addr,
			Breakpoint::MAPPING_PAGES_ALL,
			Breakpoint::Status::ACTIVE,
			true
		};

		m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_ADD, {
			{"data0", bpData.data0 },
			{"data1", bpData.data1 },
			{"data2", bpData.data2 },
			{"comment", ""}
		});

		m_hardware.Request(Hardware::Req::RUN);
	}
	ImGui::SameLine();
	if (ImGui::Button("Step 0x100") | ImGui::IsKeyPressed(ImGuiKey_F9))
	{
		for (int i = 0; i < 0x100; i++)
		{
			m_hardware.Request(Hardware::Req::STOP);
			m_hardware.Request(Hardware::Req::EXECUTE_INSTR, "100");
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Step Frame") | ImGui::IsKeyPressed(ImGuiKey_F10))
	{
		m_hardware.Request(Hardware::Req::STOP);
		m_hardware.Request(Hardware::Req::EXECUTE_FRAME_NO_BREAKS);
	}

	if (_isRunning) ImGui::EndDisabled();

	ImGui::SameLine();
	if (ImGui::Button("Reset"))
	{
		m_scheduler.AddSignal({dev::Signals::RELOAD});
	}

	ImGui::SameLine();
	dev::DrawHelpMarker(
		"Break/Run stops and continues the execution.\n"
		"Step executes one command.\n"
		"Step Over executes the next command without entering it. "
			"For example, stepping over a Call, stops the progamm at the "
			"next instruction after Call.\n"
		"Step 0x100 executes 256 instructions.\n"
		"Step Frame executes until RST7 (the next frame start).\n"
			"Reset relaods the ROM/FDD file and reset the hardware keeping "
			"all brealpoints intact."
		);
}

void dev::DisasmWindow::DrawSearch(const bool _isRunning)
{
	if (_isRunning) ImGui::BeginDisabled();
	ImGui::PushItemWidth(-100);
	if (ImGui::InputTextWithHint(
		"##disasmSearch", "FF", m_searchText, IM_ARRAYSIZE(m_searchText)))
	{
		DebugData::FilteredElements _filteredElements;
		m_debugger.GetDebugData().GetFilteredLabels(_filteredElements, m_searchText);
		int32_t addr = 0;

		if (_filteredElements.empty()){
			addr = (Addr)dev::StrCHexToInt(m_searchText);
		}
		else {
			addr = std::get<1>(_filteredElements[0]);
		};

		UpdateDisasm(addr);
		m_selectedLineAddr = addr;
	}
	if (_isRunning) ImGui::EndDisabled();

	ImGui::SameLine(); dev::DrawHelpMarker(
		"Search by a hexadecimal address in the format of 0x100 or 100,\n"
		"or by a case-sensitive label name\n\n"

		"In the disasm window below:\n"
		"Left click in the left border to add/remove breakpoints.\n"
		"Left Ctrl + left click in the left border to disable/enable breakpoints.\n"
		"Click on the highlighted instruction address/label to navigate to it.\n"
		"Left Alt + left arrow key to navigate back.\n"
		"Left Alt + right arrow key to navigate forward.\n"
		"Use the mouse wheel to scroll the program.\n"
		);
	ImGui::PopItemWidth();
}

bool dev::DisasmWindow::IsDisasmTableOutOfWindow() const
{
	ImVec2 cursorPos = ImGui::GetCursorPos();
	float remainingSpace = ImGui::GetWindowSize().y -
		cursorPos.y - ImGui::GetFontSize();

	return remainingSpace < 0;
}

int dev::DisasmWindow::GetVisibleLines() const
{
	auto lines = (ImGui::GetWindowSize().y - ImGui::GetCursorPosY()) /
				ImGui::GetFontSize();
	return static_cast<int>(lines);
}

void dev::DisasmWindow::DrawNextExecutedLineHighlight(
	const bool _isRunning, const Disasm::Line& _line, const Addr _regPC)
{
	if (_isRunning) return;
	if (_line.addr != _regPC) return;

	ImGui::SameLine();

	auto highlightMin = ImGui::GetItemRectMin();
	auto highlightMax = ImGui::GetItemRectMax();

	ImGui::GetWindowDrawList()->AddRectFilled(
		highlightMin, highlightMax, DASM_CLR_PC_LINE_HIGHLIGHT);

}

void dev::DisasmWindow::DrawDisasmIcons(
	const bool _isRunning, const Disasm::Line& _line, const int _lineIdx,
	const Addr _regPC)
{
	if (_isRunning) return;
	// draw breakpoints
	ImGui::SameLine();
	auto bpStatus = _line.breakpointStatus;
	if (dev::DrawBreakpoint(
		std::format("##BpAddr{:04d}", _lineIdx).c_str(),
		&bpStatus, *m_dpiScaleP))
	{
		switch (bpStatus)
		{
		case dev::Breakpoint::Status::DISABLED:
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DISABLE,
				{ {"addr", _line.addr} });
			break;
		case dev::Breakpoint::Status::ACTIVE:
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_ACTIVE,
				{ {"addr", _line.addr} });
			break;
		case dev::Breakpoint::Status::DELETED:
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DEL,
				{ {"addr", _line.addr} });
			break;
		}
		m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
	}

	// draw program counter icon
	if (_line.addr == _regPC)
	{
		ImGui::SameLine();
		dev::DrawProgramCounter(
			DASM_CLR_PC, ImGuiDir_Right, *m_dpiScaleP, PC_ICON_OFFSET_X);
	}
}

void dev::DisasmWindow::DrawDisasmAddr(
	const bool _isRunning, const Disasm::Line& _line,
	ContextMenu& _contextMenu, AddrHighlight& _addrHighlight)
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
		m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE, (GlobalAddr)_line.addr});
		break;
	case UIItemMouseAction::RIGHT:
		_contextMenu.Init(
			_line.addr, _line.GetAddrS(), m_debugger.GetDebugData());
		break;
	}
}

void dev::DisasmWindow::DrawDisasmCode(const bool _isRunning,
	const Disasm::Line& _line,
	ContextMenu& _contextMenu, AddrHighlight& _addrHighlight)
{
	// draw code
	ImGui::TableNextColumn();
	auto mouseAction = dev::DrawCodeLine(_isRunning, _line, true);
	// when a user did action to the immediate operand
	switch (mouseAction)
	{
	// any case below means that the immediate addr was at least hovered
	case UIItemMouseAction::LEFT: // Navigate to the address
		m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE, (GlobalAddr)_line.imm});
		break;

	// init the immediate value as an addr to let the context menu copy it
	case UIItemMouseAction::RIGHT:
		_contextMenu.Init(
			_line.imm, _line.GetImmediateS(), m_debugger.GetDebugData(), true);
		break;
	}

	// set the addr highlight when
	if (mouseAction != UIItemMouseAction::NONE) {
		_addrHighlight.Init(_line.imm);
	}
}

void dev::DisasmWindow::DrawDisasmComment(const Disasm::Line& _line)
{
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DASM_BG_CLR_ADDR);
	ImGui::TableNextColumn();

	if (m_fontCommentP) ImGui::PushFont(m_fontCommentP);
	ImGui::TextColored(DASM_CLR_COMMENT, "; %s", _line.comment->c_str());
	if (m_fontCommentP) { ImGui::PopFont(); }
}

void dev::DisasmWindow::DrawDisasmLabels(const Disasm::Line& _line)
{
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DASM_BG_CLR_ADDR);
	ImGui::TableNextColumn();
	const ImVec4* mainLabelColorP = &DASM_CLR_LABEL_GLOBAL;

	int i = 0;
	for (const auto& label : _line.labels)
	{
		if (i == 1) {
			ImGui::SameLine();
			ImGui::TextColored(DASM_CLR_COMMENT, "; ");
		}
		if (!i)
		{
			if (label[0] == '@') {
				mainLabelColorP = &DASM_CLR_LABEL_LOCAL;
			}
			ImGui::TextColored(*mainLabelColorP, label.c_str());
			ImGui::SameLine();
			ImGui::TextColored(*mainLabelColorP, ": ");
		}
		else {
			ImGui::SameLine();
			ImGui::TextColored(DASM_CLR_LABEL_MINOR, " %s", label.c_str());
		}
		if (i++ == MAX_DISASM_LABELS) {
			ImGui::SameLine();
			ImGui::TextColored(DASM_CLR_COMMENT, "...");
			break;
		}
	}
}

void dev::DisasmWindow::DrawDisasmStats(const Disasm::Line& _line)
{
	ImGui::TableNextColumn();
	ColumnClippingEnable(*m_dpiScaleP); // enable clipping
	ImGui::TableSetBgColor(
		ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(DASM_BG_CLR_ADDR));
	const ImVec4& statsColor = _line.accessed ?
							DASM_CLR_ADDR : DASM_CLR_ZERO_STATS;
	ImGui::TextColored(statsColor, _line.statsS);
	ColumnClippingDisable();
}

void dev::DisasmWindow::DrawDisasm(const bool _isRunning)
{
	auto disasmPP = m_debugger.GetDisasm().GetLines();
	if (!disasmPP || !*disasmPP) return;
	auto& disasm = **disasmPP;

	Addr regPC = m_hardware.Request(Hardware::Req::GET_REG_PC)->at("pc");
	int hoveredLineIdx = -1;
	ImVec2 selectionMin = ImGui::GetCursorScreenPos();
	ImVec2 selectionMax = ImVec2(selectionMin.x + ImGui::GetWindowWidth(),
								selectionMin.y);

	if (_isRunning) ImGui::BeginDisabled();

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5, 0 });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	if (ImGui::BeginTable("##disassembly", 5,
		ImGuiTableFlags_NoPadOuterX |
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_NoClip |
		ImGuiTableFlags_NoBordersInBodyUntilResize |
		ImGuiTableFlags_Resizable
	))
	{
		ImGui::TableSetupColumn("Brk",
			ImGuiTableColumnFlags_WidthFixed |
			ImGuiTableColumnFlags_NoResize, BRK_W);
		ImGui::TableSetupColumn("Addr",
			ImGuiTableColumnFlags_WidthFixed |
			ImGuiTableColumnFlags_NoResize, ADDR_W);
		ImGui::TableSetupColumn("command",
			ImGuiTableColumnFlags_WidthFixed, CODE_W);
		ImGui::TableSetupColumn("stats",
			ImGuiTableColumnFlags_WidthFixed, STATS_W);
		ImGui::TableSetupColumn("consts");

		m_disasmLines = dev::Min((int)disasm.size(), GetVisibleLines());

		for (int lineIdx = 0; lineIdx < m_disasmLines; lineIdx++)
		{
			auto& line = disasm.at(lineIdx);
			int addr = line.addr;

			ImGui::TableNextRow();

			// the line selection/highlight
			ImGui::TableNextColumn();
			ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DIS_BG_CLR_BRK);
			const bool isSelected = m_selectedLineAddr == addr;
			if (ImGui::Selectable(
				std::format("##disasm{}", lineIdx).c_str(),
				isSelected, ImGuiSelectableFlags_SpanAllColumns))
			{
				m_selectedLineAddr = addr;
			}
			if (!_isRunning)
			{
				selectionMin.y = ImGui::GetItemRectMin().y;
				selectionMax.y = ImGui::GetItemRectMax().y;
				hoveredLineIdx =
					ImGui::IsMouseHoveringRect(selectionMin, selectionMax) ?
					lineIdx : hoveredLineIdx;
			}

			switch (line.type)
			{
				case Disasm::Line::Type::COMMENT:
				{
					DrawDisasmComment(line);
					break;
				}
				case Disasm::Line::Type::LABELS:
				{
					DrawDisasmLabels(line);
					break;
				}
				case Disasm::Line::Type::CODE:
				{
					DrawAddrLinks(_isRunning, lineIdx, hoveredLineIdx == lineIdx);
					DrawNextExecutedLineHighlight(_isRunning, line, regPC);
					DrawDisasmIcons(_isRunning, line, lineIdx, regPC);
					DrawDisasmAddr(_isRunning, line, m_contextMenu,
						m_addrHighlight);
					DrawDisasmCode(_isRunning, line, m_contextMenu,
						m_addrHighlight);

					DrawDisasmStats(line);
					DrawDisasmConsts(line, MAX_DISASM_LABELS);
					break;
				}
			}

			if (!_isRunning)
			{
				// line is hovered. check if right-clicked to open the Context menu
				if (hoveredLineIdx == lineIdx &&
					ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					m_contextMenu.Init(
						addr, line.GetStr(), m_debugger.GetDebugData());
				}
			}
		}

		ImGui::EndTable();
	}

	DrawContextMenu(regPC, m_contextMenu);

	ImGui::PopStyleVar(2);

	/////////////////////////////////////////////////////////
	// check the keys and the mouse
	/////////////////////////////////////////////////////////
	if (!_isRunning && hoveredLineIdx >= 0)
	{
		// Up/Down scrolling
		if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
		{
			UpdateDisasm(disasm[0].addr, 2, false);
		}
		else if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
		{
			UpdateDisasm(disasm[0].addr, -2, false);
		}
		if (ImGui::GetIO().MouseWheel > 0.0f)
		{
			UpdateDisasm(disasm[0].addr, 2, false);
		}
		else if (ImGui::GetIO().MouseWheel < 0.0f)
		{
			UpdateDisasm(disasm[0].addr, -2, false);
		}


		// Alt + Left navigation
		if (ImGui::IsKeyDown(ImGuiKey_LeftAlt) &&
			ImGui::IsKeyPressed(ImGuiKey_LeftArrow) &&
			m_navigateAddrsIdx - 1 >= 0)
		{
			auto addr = m_navigateAddrs[--m_navigateAddrsIdx];
			UpdateDisasm(addr);
		}
		// Alt + Right navigation
		else if (ImGui::IsKeyDown(ImGuiKey_LeftAlt) &&
			ImGui::IsKeyPressed(ImGuiKey_RightArrow) &&
			m_navigateAddrsIdx + 1 < m_navigateAddrsSize)
		{
			auto addr = m_navigateAddrs[++m_navigateAddrsIdx];
			UpdateDisasm(addr);
		}
		// Left side navigation mouse button to navigate back
		else if (ImGui::IsKeyPressed(ImGuiKey_MouseX1) &&
			m_navigateAddrsIdx - 1 >= 0)
		{
			auto addr = m_navigateAddrs[--m_navigateAddrsIdx];
			UpdateDisasm(addr);
		}
		// Right side navigation mouse button to navigate forward
		else if (ImGui::IsKeyPressed(ImGuiKey_MouseX2) &&
			m_navigateAddrsIdx + 1 < m_navigateAddrsSize)
		{
			auto addr = m_navigateAddrs[++m_navigateAddrsIdx];
			UpdateDisasm(addr);
		}
	}

	if (_isRunning) ImGui::EndDisabled();
}

void dev::DisasmWindow::UpdateDisasm(
	const Addr _addr, const int _instructionsOffset,
	const bool _updateSelection)
{
	m_disasmAddr = _addr;
	m_debugger.UpdateDisasm(_addr, m_disasmLines, -_instructionsOffset);
	m_immLinksP = m_debugger.GetDisasm().GetImmLinks();
	m_immLinksNum = m_debugger.GetDisasm().GetImmAddrlinkNum();

	if (_updateSelection) m_selectedLineAddr = _addr;
}

void dev::DisasmWindow::DrawContextMenu(
	const Addr _regPC, ContextMenu& _contextMenu)
{
	// draw a context menu
	if (_contextMenu.BeginPopup())
	{
		if (ImGui::MenuItem("Copy")) {
			dev::CopyToClipboard(_contextMenu.str);
		}
		ImGui::SeparatorText("");
		if (ImGui::MenuItem("Show Current Break")) {
			UpdateDisasm(_regPC);
		}
		ImGui::SeparatorText("");
		if (ImGui::MenuItem("Run To") | (
			ImGui::IsKeyPressed(ImGuiKey_F7) &&
			ImGui::IsKeyDown(ImGuiKey_LeftCtrl)))
		{
			Breakpoint::Data bpData{
				_contextMenu.addr, Breakpoint::MAPPING_PAGES_ALL,
				Breakpoint::Status::ACTIVE, true };
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_ADD, {
				{"data0", bpData.data0 },
				{"data1", bpData.data1 },
				{"data2", bpData.data2 },
				{"comment", ""}
			});

			m_hardware.Request(Hardware::Req::RUN);
		}
		ImGui::SeparatorText("");
		if (ImGui::MenuItem("Add/Remove Beakpoint"))
		{
			Breakpoint::Status bpStatus = static_cast<Breakpoint::Status>(
				m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_GET_STATUS,
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
				m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DEL,
					{ {"addr", m_contextMenu.addr} });
			}
			m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
		}
		if (ImGui::MenuItem("Remove All Beakpoints")) {
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DEL_ALL);
			m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
		};

		ImGui::SeparatorText("");

		if (ImGui::MenuItem("Add/Edit Label"))
		{
			if (_contextMenu.labelExists)
			{
				m_scheduler.AddSignal({
					dev::Signals::LABEL_EDIT_WINDOW_EDIT,
					(Addr)_contextMenu.addr});
			}
			else
			{
				m_scheduler.AddSignal({
					dev::Signals::LABEL_EDIT_WINDOW_ADD,
					(Addr)_contextMenu.addr});
			}

		};

		if (_contextMenu.immHovered && ImGui::MenuItem("Add/Edit Const"))
		{
			if (_contextMenu.constExists)
			{
				m_scheduler.AddSignal({
					dev::Signals::CONST_EDIT_WINDOW_EDIT,
					(Addr)_contextMenu.addr});
			}
			else
			{
				m_scheduler.AddSignal({
					dev::Signals::CONST_EDIT_WINDOW_ADD,
					(Addr)_contextMenu.addr});
			}
		};

		if (ImGui::MenuItem("Add/Edit Comment"))
		{
			if (_contextMenu.commentExists)
			{
				m_scheduler.AddSignal({
					dev::Signals::COMMENT_EDIT_WINDOW_EDIT,
					(Addr)_contextMenu.addr});
			}
			else
			{
				m_scheduler.AddSignal({
					dev::Signals::COMMENT_EDIT_WINDOW_ADD,
					(Addr)_contextMenu.addr});
			}
		};

		ImGui::SeparatorText("");

		if (ImGui::MenuItem("Edit Memory"))
		{
			if (_contextMenu.editMemoryExists)
			{
				m_scheduler.AddSignal({
					dev::Signals::MEMORY_EDIT_WINDOW_EDIT,
					(Addr)_contextMenu.addr});
			}
			else
			{
				m_scheduler.AddSignal({
					dev::Signals::MEMORY_EDIT_WINDOW_ADD,
					(Addr)_contextMenu.addr});
			}
		}

		if (_contextMenu.editMemoryExists && ImGui::MenuItem(
			"Cancel Edit Memory at This Addr"))
		{
			m_hardware.Request(
				Hardware::Req::DEBUG_MEMORY_EDIT_DEL,
				{ {"addr", _contextMenu.addr} });

			m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
		}

		if (ImGui::MenuItem("Code Perf"))
		{
			if (_contextMenu.codePerfExists)
			{
				m_scheduler.AddSignal({
					dev::Signals::CODE_PERF_EDIT_WINDOW_EDIT,
					(Addr)_contextMenu.addr});
			}
			else
			{
				m_scheduler.AddSignal({
					dev::Signals::CODE_PERF_EDIT_WINDOW_ADD,
					(Addr)_contextMenu.addr});
			}
		}

		ImGui::EndPopup();
	}
}

void dev::DisasmWindow::DrawAddrLinks(
	const bool _isRunning, const int _lineIdx,
	const bool _selected)
{
	if (_isRunning) return;

	auto& link = m_immLinksP->at(_lineIdx);
	if (link.lineIdx == Disasm::IMM_NO_LINK) return;

	float linkWidth = _selected ? 1.5f : 1.0f;

	float linkHorizLen = IMM_ADDR_LINK_AREA_W * link.linkIdx / m_immLinksNum;

	auto pos0 = ImGui::GetCursorScreenPos();
	pos0.x += IMM_ADDR_LINK_POS_X;
	pos0.y -= ImGui::GetFontSize() * 0.5f;
	auto pos1 = ImVec2(pos0.x - linkHorizLen - 5.0f, pos0.y);

	bool minorLink = (link.lineIdx == Disasm::IMM_LINK_UP ||
					link.lineIdx == Disasm::IMM_LINK_DOWN ||
					link.lineIdx == Disasm::IMM_NO_LINK);
	ImU32 linkColor = _selected ? DIS_CLR_LINK_HIGHLIGHT : DIS_CLR_LINK;

	if (minorLink)
	{
		// TODO: check if it's needed
		// links to the addrs outside the disasm view
		/*
		// horizontal line from the command
		ImGui::GetForegroundDrawList()->AddLine(pos0, pos1, linkColor);
		// vertical link
		if (link.m_lineIdx == Disasm::IMM_LINK_UP) {
			ImGui::GetForegroundDrawList()->AddLine(pos1, ImVec2(pos0.x - linkHorizLen - 5.0f, _posMin), linkColor);
		}
		else if (link.m_lineIdx == Disasm::IMM_LINK_DOWN) {
			ImGui::GetForegroundDrawList()->AddLine(pos1, ImVec2(pos0.x - linkHorizLen - 5.0f, _posMax), linkColor);
		}
		*/
	}
	else {
		// horizontal line from the command
		ImGui::GetForegroundDrawList()->AddLine(pos0, pos1, linkColor, linkWidth);
		// vertical line
		auto pos2 = ImVec2(
			pos1.x,
			pos1.y + (link.lineIdx - _lineIdx) * ImGui::GetFontSize());
		ImGui::GetForegroundDrawList()->AddLine(
			pos1, pos2, linkColor, linkWidth);
		// horizontal line to the addr
		pos0.y = pos2.y;
		ImGui::GetForegroundDrawList()->AddLine(
			pos2, pos0, linkColor, linkWidth);
		// arrow
		float r = 5.0f;
		ImVec2 center = pos0;
		auto a = ImVec2(center.x + 0.750f * r, center.y);
		auto b = ImVec2(center.x - 0.750f * r, center.y + 0.866f * r);
		auto c = ImVec2(center.x - 0.750f * r, center.y - 0.866f * r);
		ImGui::GetForegroundDrawList()->AddTriangleFilled(a, b, c, linkColor);
	}
}


void dev::DisasmWindow::CallbackUpdateAtCC(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	// update
	Addr addr = m_hardware.Request(Hardware::Req::GET_REG_PC)->at("pc");
	UpdateDisasm(addr);
}


void dev::DisasmWindow::CallbackUpdateAtAddr(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
    if (_data.has_value() && std::holds_alternative<GlobalAddr>(*_data))
	{
        auto addr = Addr(std::get<GlobalAddr>(*_data));

		UpdateDisasm(addr);

		if (m_navigateAddrsIdx == 0)
		{
			auto disasmPP = m_debugger.GetDisasm().GetLines();
			if (!disasmPP || !*disasmPP) return;
			m_navigateAddrs[m_navigateAddrsIdx] =
				(*disasmPP)->at(DISASM_INSTRUCTION_OFFSET).addr;
			m_navigateAddrsSize++;
		}
		if (m_navigateAddrsIdx < NAVIGATE_ADDRS_LEN)
		{
			m_navigateAddrs[++m_navigateAddrsIdx] = addr;
			m_navigateAddrsSize = m_navigateAddrsIdx + 1;
		}
	}
	else{
		UpdateDisasm(m_disasmAddr, DISASM_INSTRUCTION_OFFSET, false);
	}
}