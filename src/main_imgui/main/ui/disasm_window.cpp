#include <format>

#include "ui/disasm_window.h"
#include "utils/str_utils.h"
#include "imgui_stdlib.h"

dev::DisasmWindow::DisasmWindow(
		dev::Hardware& _hardware, Debugger& _debugger, ImFont* fontComment,
		dev::Scheduler& _scheduler,
		bool* _visibleP)
	:
	BaseWindow("Disasm", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP),
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
		// Persist current debug data before reloading the ROM/FDD
		m_debugger.GetDebugData().SaveDebugData();
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



void dev::DisasmWindow::DrawDisasm(const bool _isRunning)
{
	auto& lines = m_debugger.GetDisasm().GetLines();
	if (lines.empty()) return;

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
		auto scale = ImGui::GetWindowDpiScale();

		ImGui::TableSetupColumn("Brk",
			ImGuiTableColumnFlags_WidthFixed |
			ImGuiTableColumnFlags_NoResize, BRK_W * scale);
		ImGui::TableSetupColumn("Addr",
			ImGuiTableColumnFlags_WidthFixed |
			ImGuiTableColumnFlags_NoResize, ADDR_W * scale);
		ImGui::TableSetupColumn("command",
			ImGuiTableColumnFlags_WidthFixed, CODE_W * scale);
		ImGui::TableSetupColumn("stats",
			ImGuiTableColumnFlags_WidthFixed, STATS_W * scale);
		ImGui::TableSetupColumn("consts");

		m_disasmLines = dev::Min((int)lines.size(), GetVisibleLines());

		auto line_iter = lines.begin();

		for (int lineIdx = 0; lineIdx < m_disasmLines; lineIdx++)
		{
			const auto& line = *line_iter;
			line_iter++;
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
				case DisasmLine::Type::COMMENT:
				{
					DrawDisasmComment(line);
					break;
				}
				case DisasmLine::Type::LABELS:
				{
					DrawDisasmLabels(line);
					break;
				}
				case DisasmLine::Type::CODE:
				{
					DrawAddrLinks(_isRunning, lineIdx, hoveredLineIdx == lineIdx);
					DrawNextExecutedLineHighlight(_isRunning, line, regPC);
					DrawDisasmIcons(_isRunning, line, lineIdx, regPC);
					DrawDisasmAddr(_isRunning, line, m_addrHighlight);
					DrawDisasmCode(_isRunning, line, m_addrHighlight);

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
					m_scheduler.AddSignal({
						dev::Signals::DISASM_POPUP_OPEN, (GlobalAddr)addr});
				}
			}
		}

		ImGui::EndTable();
	}


	ImGui::PopStyleVar(2);
	if (_isRunning) ImGui::EndDisabled();

	if (!_isRunning && hoveredLineIdx >= 0){
		CheckControls(lines.begin()->addr);
	}
}


void dev::DisasmWindow::CheckControls(const Addr _addr)
{
	// Up/Down scrolling
	if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
	{
		UpdateDisasm(_addr, 2, false);
	}
	else if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
	{
		UpdateDisasm(_addr, -2, false);
	}
	// Mouse wheel scrolling
	if (ImGui::GetIO().MouseWheel > 0.0f)
	{
		UpdateDisasm(_addr, 2, false);
	}
	else if (ImGui::GetIO().MouseWheel < 0.0f)
	{
		UpdateDisasm(_addr, -2, false);
	}

	// Alt + Left navigation
	if (ImGui::IsKeyDown(ImGuiKey_LeftAlt) &&
		ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
	{
		auto addrR = m_navigateHistory.GetPrev();
		if (addrR) UpdateDisasm(*addrR);
	}
	// Alt + Right navigation
	else if (ImGui::IsKeyDown(ImGuiKey_LeftAlt) &&
		ImGui::IsKeyPressed(ImGuiKey_RightArrow))
	{
		auto addrR = m_navigateHistory.GetNext();
		if (addrR) UpdateDisasm(*addrR);
	}
	// Left side navigation mouse button to navigate back
	else if (ImGui::IsKeyPressed(ImGuiKey_MouseX1))
	{
		auto addrR = m_navigateHistory.GetPrev();
		if (addrR) UpdateDisasm(*addrR);
	}
	// Right side navigation mouse button to navigate forward
	else if (ImGui::IsKeyPressed(ImGuiKey_MouseX2))
	{
		auto addrR = m_navigateHistory.GetNext();
		if (addrR) UpdateDisasm(*addrR);
	}
}


void dev::DisasmWindow::UpdateDisasm(
	const Addr _addr, const int _instructionsOffset,
	const bool _updateSelection)
{
	m_disasmAddr = _addr;
	m_debugger.GetDisasm().UpdateDisasm(_addr, m_disasmLines, -_instructionsOffset);
	m_immLinksP = m_debugger.GetDisasm().GetImmLinks();

	if (_updateSelection) m_selectedLineAddr = _addr;
}


void dev::DisasmWindow::CallbackUpdateAtCC(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	// update
	Addr addr = m_hardware.Request(Hardware::Req::GET_REG_PC)->at("pc");
	UpdateDisasm(addr);
	m_navigateHistory.Add(addr);
}


void dev::DisasmWindow::CallbackUpdateAtAddr(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
    if (_data.has_value() && std::holds_alternative<GlobalAddr>(*_data))
	{
        auto addr = Addr(std::get<GlobalAddr>(*_data));

		UpdateDisasm(addr);
		m_navigateHistory.Add(addr);
	}
	else{
		UpdateDisasm(m_disasmAddr, DISASM_INSTRUCTION_OFFSET, false);
		m_navigateHistory.Add(m_disasmAddr);
	}
}

int dev::DisasmWindow::GetVisibleLines() const
{
	auto lines = (ImGui::GetWindowSize().y - ImGui::GetCursorPosY()) /
				ImGui::GetFontSize();
	return static_cast<int>(lines);
}

void dev::DisasmWindow::DrawAddrLink(
	ImVec2 _pos0,
	ImVec2 _pos1,
	int _lineIdx,
	ImU32 _linkColor,
	int _endLineIdx,
	float _linkWidth)
{
	// horizontal line from the command
	ImGui::GetForegroundDrawList()->AddLine(
		_pos0, _pos1, _linkColor, _linkWidth);

	// vertical line
	auto pos2 = ImVec2(
		_pos1.x,
		_pos1.y + (_endLineIdx - _lineIdx) * ImGui::GetFontSize());
	ImGui::GetForegroundDrawList()->AddLine(
		_pos1, pos2, _linkColor, _linkWidth);
	// horizontal line to the addr
	_pos0.y = pos2.y;
	ImGui::GetForegroundDrawList()->AddLine(
		pos2, _pos0, _linkColor, _linkWidth);
	// arrow
	float r = 5.0f;
	ImVec2 center = _pos0;
	auto a = ImVec2(center.x + 0.750f * r, center.y);
	auto b = ImVec2(center.x - 0.750f * r, center.y + 0.866f * r);
	auto c = ImVec2(center.x - 0.750f * r, center.y - 0.866f * r);
	ImGui::GetForegroundDrawList()->AddTriangleFilled(a, b, c, _linkColor);
}

void dev::DisasmWindow::DrawAddrLinks(
	const bool _isRunning,
	const int _lineIdx,
	const bool _selected)
{
	if (_isRunning || !m_immLinksP) return;

	auto link_iter = m_immLinksP->find(_lineIdx);
	if (link_iter == m_immLinksP->end()) return;
	const auto& link = link_iter->second;

	float linkWidth = _selected ? 2.0f : 0.8f;

	float linkHorizLen = IMM_ADDR_LINK_AREA_W * link.linkIdx / m_immLinksP->size();

	auto pos0 = ImGui::GetCursorScreenPos();
	pos0.x += IMM_ADDR_LINK_POS_X;
	pos0.y -= ImGui::GetFontSize() * 0.5f;
	auto pos1 = ImVec2(pos0.x - linkHorizLen - 5.0f, pos0.y);

	ImU32 linkColor = _selected ? DIS_CLR_LINK_HIGHLIGHT : DIS_CLR_LINK;

	if (link.endLineIdx == Disasm::IMM_LINK_UP ||
		link.endLineIdx == Disasm::IMM_LINK_DOWN)
	{
		// TODO: check if it's needed
		// links to the addrs outside the disasm view

		// horizontal line from the command
		// ImGui::GetForegroundDrawList()->AddLine(pos0, pos1, linkColor);
		// vertical link
		// if (link.m_lineIdx == Disasm::IMM_LINK_UP) {
		// 	ImGui::GetForegroundDrawList()->AddLine(pos1, ImVec2(pos0.x - linkHorizLen - 5.0f, _posMin), linkColor);
		// }
		// else if (link.m_lineIdx == Disasm::IMM_LINK_DOWN) {
		//	ImGui::GetForegroundDrawList()->AddLine(pos1, ImVec2(pos0.x - linkHorizLen - 5.0f, _posMax), linkColor);
		// }
	}
	else {
		// draw shadow
		ImVec2 pos0_shadow = ImVec2(pos0.x + linkWidth, pos0.y + linkWidth);
		ImVec2 pos1_shadow = ImVec2(pos1.x + linkWidth, pos1.y + linkWidth);
		DrawAddrLink(
			pos0, pos1, _lineIdx, dev::IM_U32(0x00000070), link.endLineIdx, linkWidth* 2.0f);
		// draw link
		DrawAddrLink(
			pos0, pos1, _lineIdx, linkColor, link.endLineIdx, linkWidth);
	}
}


void dev::DisasmWindow::DrawNextExecutedLineHighlight(
	const bool _isRunning, const DisasmLine& _line, const Addr _regPC)
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
	const bool _isRunning, const DisasmLine& _line, const int _lineIdx,
	const Addr _regPC)
{
	if (_isRunning) return;

	// draw breakpoints
	ImGui::SameLine();
	auto bpStatus = _line.breakpointStatus;
	if (dev::DrawBreakpoint(
		std::format("##BpAddr{:04d}", _lineIdx).c_str(),
		&bpStatus))
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
			DASM_CLR_PC, ImGuiDir_Right, PC_ICON_OFFSET_X);
	}
}



void dev::DisasmWindow::DrawDisasmAddr(
	const bool _isRunning,
	const DisasmLine& _line,
	AddrHighlight& _addrHighlight)
{
	// the addr column
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DASM_BG_CLR_ADDR);

	auto mouseAction = DrawAddr(_isRunning, Uint16ToStrC0x(_line.addr),
		DASM_CLR_LABEL_MINOR, dev::IM_VEC4(0xFFFFFFFF),
		_addrHighlight.IsEnabled(_line.addr));
	switch (mouseAction)
	{
	case UIItemMouseAction::LEFT: // Navigate to the address
		m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE, (GlobalAddr)_line.addr});
		break;
	case UIItemMouseAction::RIGHT:
		m_scheduler.AddSignal({
			dev::Signals::DISASM_POPUP_OPEN, (GlobalAddr)_line.addr});
		break;
	}
}

void dev::DisasmWindow::DrawDisasmCode(const bool _isRunning,
	const DisasmLine& _line, AddrHighlight& _addrHighlight)
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
		m_scheduler.AddSignal({
			dev::Signals::DISASM_POPUP_OPEN_IMM, (GlobalAddr)_line.imm});
		break;
	}

	// set the addr highlight when
	if (mouseAction != UIItemMouseAction::NONE) {
		_addrHighlight.Init(_line.imm);
	}
}

void dev::DisasmWindow::DrawDisasmComment(const DisasmLine& _line)
{
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DASM_BG_CLR_ADDR);
	ImGui::TableNextColumn();

	if (m_fontCommentP) ImGui::PushFont(m_fontCommentP);
	ImGui::TextColored(DASM_CLR_COMMENT, "; %s", _line.comment.c_str());
	if (m_fontCommentP) { ImGui::PopFont(); }
}

void dev::DisasmWindow::DrawDisasmLabels(const DisasmLine& _line)
{
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DASM_BG_CLR_ADDR);
	ImGui::TableNextColumn();

	const auto& label = _line.label;
	if (label.empty()) return;

	const ImVec4* mainLabelColorP = label[0] != '@' ?
									&DASM_CLR_LABEL_GLOBAL :
									&DASM_CLR_LABEL_LOCAL;

	ImGui::TextColored(*mainLabelColorP, "%s", label.c_str());

	ImGui::SameLine();
	ImGui::TextColored(DASM_CLR_LABEL_MINOR, ":");

	ImGui::SameLine();
	ImGui::TextColored(DASM_CLR_COMMENT, _line.post_comment.c_str());
}

void dev::DisasmWindow::DrawDisasmStats(const DisasmLine& _line)
{
	ImGui::TableNextColumn();
	ColumnClippingEnable(); // enable clipping
	ImGui::TableSetBgColor(
		ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(DASM_BG_CLR_ADDR));
	const ImVec4& statsColor = _line.accessed ?
							DASM_CLR_ADDR : DASM_CLR_ZERO_STATS;
	ImGui::TextColored(statsColor, _line.stats.c_str());
	ColumnClippingDisable();
}