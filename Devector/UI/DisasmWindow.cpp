#include <format>
#include "DisasmWindow.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/StrUtils.h"


dev::DisasmWindow::DisasmWindow(
		dev::Hardware& _hardware, Debugger& _debugger, ImFont* fontComment,
		const float* const _fontSize, const float* const _dpiScale, 
		ReqDisasm& _reqDisasm, bool& _reset, bool& _reload)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSize, _dpiScale),
	m_hardware(_hardware),
	m_debugger(_debugger),
	m_fontCommentP(fontComment),
	m_reqDisasm(_reqDisasm),
	m_reqHardwareStatsReset(_reset), m_navigateAddrs(),
	m_reqMainWindowReload(_reload)
{
	UpdateData(false);
}

void dev::DisasmWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Disasm", &open, ImGuiWindowFlags_NoCollapse);

	bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");

	DrawDebugControls(isRunning);
	DrawSearch(isRunning);


	isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning"); // in case it changed the bpStatus in DrawDebugControls
	UpdateData(isRunning);
	DrawDisasm(isRunning);

	ImGui::End();
}

void dev::DisasmWindow::DrawDebugControls(const bool _isRunning)
{
	if (!_isRunning && ImGui::Button("Conti"))
	{
		m_hardware.Request(Hardware::Req::RUN);
	}
	else if (_isRunning && ImGui::Button("Break"))
	{
		m_hardware.Request(Hardware::Req::STOP);
	}

	if (_isRunning) ImGui::BeginDisabled();

	 ImGui::SameLine();
	if (ImGui::Button("Step"))
	{
		m_hardware.Request(Hardware::Req::STOP);
		m_hardware.Request(Hardware::Req::EXECUTE_INSTR);
	}
	ImGui::SameLine();
	if (ImGui::Button("Step 0x100"))
	{
		for (int i = 0; i < 0x100; i++)
		{
			m_hardware.Request(Hardware::Req::STOP);
			m_hardware.Request(Hardware::Req::EXECUTE_INSTR, "100");
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Step Frame"))
	{
		m_hardware.Request(Hardware::Req::STOP);
		m_hardware.Request(Hardware::Req::EXECUTE_FRAME_NO_BREAKS);
	}

	if (_isRunning) ImGui::EndDisabled();

	ImGui::SameLine();
	if (ImGui::Button("Reset"))
	{
		m_reqHardwareStatsReset = true;
		m_reqMainWindowReload = true;
		m_hardware.Request(Hardware::Req::STOP);
	}
}

void dev::DisasmWindow::DrawSearch(const bool _isRunning)
{
	if (_isRunning) ImGui::BeginDisabled();
	ImGui::PushItemWidth(-100);
	if (ImGui::InputTextWithHint("##disasmSearch", "FF", m_searchText, IM_ARRAYSIZE(m_searchText), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		Addr addr = (Addr)dev::StrHexToInt(m_searchText);
		UpdateDisasm(addr);
		m_selectedLineIdx = DISASM_INSTRUCTION_OFFSET;
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
	float remainingSpace = ImGui::GetWindowSize().y - cursorPos.y - *m_fontSizeP;

	return remainingSpace < 0;
}

int dev::DisasmWindow::GetVisibleLines() const
{
	auto lines = (ImGui::GetWindowSize().y - ImGui::GetCursorPos().y) / (*m_fontSizeP);
	return static_cast<int>(lines);
}

void dev::DisasmWindow::DrawDsasmIcons(const bool _isRunning, const Disasm::Line& _line, const int _lineIdx, const Addr _regPC)
{
	if (!_isRunning) {
		// draw breakpoints
		ImGui::SameLine();
		auto bpStatus = _line.breakpointStatus;
		if (dev::DrawBreakpoint(std::format("##BpAddr{:04d}", _lineIdx).c_str(), &bpStatus, *m_dpiScaleP))
		{
			if (bpStatus == Breakpoint::Status::DELETED) {
				m_debugger.DelBreakpoint(_line.addr);
			}
			else m_debugger.SetBreakpointStatus(_line.addr, bpStatus);
			m_reqDisasm.type = ReqDisasm::Type::UPDATE;
		}

		// draw program counter icon
		if (_line.addr == _regPC)
		{
			ImGui::SameLine();
			dev::DrawProgramCounter(DASM_CLR_PC, ImGuiDir_Right, *m_dpiScaleP);
		}
	}
}

auto dev::DisasmWindow::DrawDisasmAddr(const bool _isRunning, const Disasm::Line& _line, const uint8_t _addrHighlightedAlpha,
	int& _copyToClipboardAddr, bool& _openItemContextMenu)
-> ImVec2
{
	// the addr column
	ImGui::TableNextColumn();
	ColumnClippingEnable(*m_dpiScaleP); // enable clipping
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DASM_BG_CLR_ADDR);

	dev::DrawAddr(_isRunning, _line.GetAddrS(), _addrHighlightedAlpha,
		// _onMouseLeft. Navigate to the address
		[&]()
		{
			m_reqDisasm.type = ReqDisasm::Type::NAVIGATE_TO_ADDR;
			m_reqDisasm.addr = _line.addr;
		},
		// _onMouseRight. Add the "Copy to Clipboard" option to the context menu
		[&]()
		{
			_copyToClipboardAddr = _line.addr;
			_openItemContextMenu = true;
		}
	);
	ColumnClippingDisable();
	
	return ImGui::GetItemRectMin();
}

void dev::DisasmWindow::DrawDisasmCode(const bool _isRunning, const Disasm::Line& _line,
	int& _addrHighlighted, int& _addrHighlightedTimer, int& _copyToClipboardAddr, bool& _openItemContextMenu)
{
	// draw code
	ImGui::TableNextColumn();
	ColumnClippingEnable(*m_dpiScaleP); // enable clipping
	int currentLineAddrHighlighted = dev::DrawCodeLine2(true, _isRunning, _line,
		// _onMouseLeft. Navigate to the address
		[&](const Addr _addr)
		{
			m_reqDisasm.type = ReqDisasm::Type::NAVIGATE_TO_ADDR;
			m_reqDisasm.addr = _addr;
		},
		// _onMouseRight. Add the "Copy to Clipboard" option to the context menu
		[&](const Addr _addr)
		{
			_copyToClipboardAddr = _addr;
			_openItemContextMenu = true;
		}
	);
	// handle the addr highlight
	if (currentLineAddrHighlighted >= 0) {
		// set the highlighted addr
		_addrHighlightedTimer = ADDR_HIGHLIGHT_TIME;
		_addrHighlighted = currentLineAddrHighlighted;
	}
	else {
		// handle the addr highlight timer
		_addrHighlightedTimer = dev::Max(0, --_addrHighlightedTimer);
	}
	ColumnClippingDisable();
}

auto dev::DisasmWindow::DrawDisasmComment(const Disasm::Line& _line)
-> ImVec2
{
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DASM_BG_CLR_ADDR);
	ImGui::TableNextColumn();
	auto selectionMin = ImGui::GetItemRectMin();

	if (m_fontCommentP) ImGui::PushFont(m_fontCommentP);
	ImGui::TextColored(DASM_CLR_COMMENT, _line.comments->at(0).c_str());
	if (m_fontCommentP) { ImGui::PopFont(); }

	return selectionMin;
}

auto dev::DisasmWindow::DrawDisasmLabels(const Disasm::Line& _line)
-> ImVec2
{
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DASM_BG_CLR_ADDR);
	ImGui::TableNextColumn();
	auto selectionMin = ImGui::GetItemRectMin();
	const ImVec4* mainLabelColorP = &DASM_CLR_LABEL_GLOBAL;

	if (!_line.labels) return selectionMin;

	int i = 0;
	for (const auto& label : *_line.labels) 
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

	return selectionMin;
}

void dev::DisasmWindow::DrawDisasmStats(const Disasm::Line& _line)
{
	ImGui::TableNextColumn();
	ColumnClippingEnable(*m_dpiScaleP); // enable clipping
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(DASM_BG_CLR_ADDR));
	const ImVec4& statsColor = _line.accessed ? DASM_CLR_ADDR : DASM_CLR_ZERO_STATS;
	ImGui::TextColored(statsColor, _line.statsS);
	ColumnClippingDisable();
}

void dev::DisasmWindow::DrawDisasmConsts(const Disasm::Line& _line)
{
	ImGui::TableNextColumn();
	if (!_line.consts) return;

	int i = 0;
	for (const auto& const_ : *_line.consts)
	{
		if (i) {
			ImGui::SameLine();
			ImGui::TextColored(DASM_CLR_COMMENT, ", ");
		}
		ImGui::TextColored(DASM_CLR_ADDR, const_.c_str());

		if (i++ == MAX_DISASM_LABELS) {
			ImGui::SameLine();
			ImGui::TextColored(DASM_CLR_COMMENT, "...");
			break;
		}
	}
}

void dev::DisasmWindow::DrawDisasm(const bool _isRunning)
{
	Addr regPC = m_hardware.Request(Hardware::Req::GET_REGS)->at("pc");
	bool openItemContextMenu = false;
	static int itemContextMenuAddr = -1;
	static std::string copyToClipboardStr = "";
	static int copyToClipboardAddr = -1; // if it's -1, don't add the option, if it's >=0, add the option with the addr = copyToClipboardAddr
	static int addrHighlighted = -1; // -1 - no highlight, >=0 - the addrHighlighted is highlighted
	static int addrHighlightedTimer = 0; // the highlight slowly decays during addrHighlightedTimer

	ImVec2 selectionMin;
	ImVec2 selectionMax = {-1.0, -1.0};

	if (!m_disasmP) return;
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
		ImGui::TableSetupColumn("Brk", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, BRK_W);
		ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, ADDR_W);
		ImGui::TableSetupColumn("command", ImGuiTableColumnFlags_WidthFixed, CODE_W);
		ImGui::TableSetupColumn("stats", ImGuiTableColumnFlags_WidthFixed, STATS_W);
		ImGui::TableSetupColumn("consts");

		m_disasmLines = dev::Min((int)m_disasmP->size(), GetVisibleLines());

		for (int lineIdx = 0; lineIdx < m_disasmLines; lineIdx++)
		{
			// TODO: fix it. replace with fixed amount of lines and without a need to check the end of the window
			//if (IsDisasmTableOutOfWindow()) break;

			ImGui::TableNextRow();

			auto& line = m_disasmP->at(lineIdx);
			int addr = line.addr;

			// the line selection/highlight
			ImGui::TableNextColumn();
			ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DIS_BG_CLR_BRK);
			const bool isSelected = m_selectedLineIdx == lineIdx;
			if (ImGui::Selectable(std::format("##disasm{}", lineIdx).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
			{
				m_selectedLineIdx = lineIdx;
			}

			switch (line.type)
			{
			case Disasm::Line::Type::COMMENT:
			{
				selectionMin = DrawDisasmComment(line);
				break;
			}
			case Disasm::Line::Type::LABELS:
			{
				selectionMin = DrawDisasmLabels(line);
				break;
			}
			case Disasm::Line::Type::CODE:
			{
				DrawDsasmIcons(_isRunning, line, lineIdx, regPC);
				uint8_t addrHighlightedAlpha = (addrHighlighted == line.addr) ? addrHighlightedTimer * 255 / ADDR_HIGHLIGHT_TIME : 0;
				selectionMin = DrawDisasmAddr(_isRunning, line, addrHighlightedAlpha, copyToClipboardAddr, openItemContextMenu);
				DrawDisasmCode(_isRunning, line, addrHighlighted, addrHighlightedTimer, copyToClipboardAddr, openItemContextMenu);
				DrawDisasmStats(line);
				DrawDisasmConsts(line);
				selectionMax = ImGui::GetItemRectMax();
				break;
			}
			default:
				break;
			}

			// check if right-click on the row
			if (!_isRunning && selectionMax.x != -1.0f && selectionMax.y != -1.0f &&
				ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				auto mousePos = ImGui::GetMousePos();
				if (mousePos.x >= selectionMin.x && mousePos.x < selectionMax.x &&
					mousePos.y >= selectionMin.y && mousePos.y < selectionMax.y)
				{
					if (!openItemContextMenu) {
						openItemContextMenu = true;
						copyToClipboardAddr = -1;
					}
					itemContextMenuAddr = addr;
					copyToClipboardStr = line.GetStr();
				}
			}
		}

		ImGui::EndTable();
	}
	copyToClipboardAddr = DrawDisasmContextMenu(openItemContextMenu, regPC, itemContextMenuAddr,
		copyToClipboardAddr, copyToClipboardStr);

	ImGui::PopStyleVar(2);

	/////////////////////////////////////////////////////////	
	// check the keys
	/////////////////////////////////////////////////////////	
	if (ImGui::IsItemHovered() &&
		m_disasmLines >= DISASM_INSTRUCTION_OFFSET)
	{
		// TODO: BUG: the code below only handles up and down arrow keys once for some reason
		/*
		if (ImGui::IsKeyDown(ImGuiKey_PageUp))
		{
			m_selectedLineIdx = dev::Min(m_selectedLineIdx + 1, lineIdx - 1);
			UpdateDisasm(m_disasmP->at(DISASM_INSTRUCTION_OFFSET).addr, 1 + DISASM_INSTRUCTION_OFFSET, false);
		}
		else if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
		{
			{
				m_selectedLineIdx += 1;
				if (m_selectedLineIdx > lineIdx - 1) {
					m_selectedLineIdx = lineIdx - 1;
					UpdateDisasm(m_disasmP->at(0).addr, -1 + DISASM_INSTRUCTION_OFFSET, false);
				}
			}
		}
		*/
		if (ImGui::GetIO().MouseWheel > 0.0f)
		{
			m_selectedLineIdx = dev::Min(m_selectedLineIdx + 2, m_disasmLines - 1);
			UpdateDisasm(m_disasmP->at(DISASM_INSTRUCTION_OFFSET).addr, 2 + DISASM_INSTRUCTION_OFFSET, false);
		}
		else if (ImGui::GetIO().MouseWheel < 0.0f)
		{
			m_selectedLineIdx = dev::Max(m_selectedLineIdx - 2, 0);
			UpdateDisasm(m_disasmP->at(DISASM_INSTRUCTION_OFFSET).addr, -2 + DISASM_INSTRUCTION_OFFSET, false);
		}
	}

	// Alt + Left navigation
	if (ImGui::IsKeyDown(ImGuiKey_LeftAlt) && ImGui::IsKeyPressed(ImGuiKey_LeftArrow) &&
		m_navigateAddrsIdx - 1 >= 0)
	{
		auto addr = m_navigateAddrs[--m_navigateAddrsIdx];
		UpdateDisasm(addr);
	}
	// Alt + Right navigation
	else if (ImGui::IsKeyDown(ImGuiKey_LeftAlt) && ImGui::IsKeyPressed(ImGuiKey_RightArrow) &&
		m_navigateAddrsIdx + 1 < m_navigateAddrsSize)
	{
		auto addr = m_navigateAddrs[++m_navigateAddrsIdx];
		UpdateDisasm(addr);
	}

	/////////////////////////////////////////////////////////
	// request handling
	/////////////////////////////////////////////////////////
	if (!_isRunning && m_reqDisasm.type != ReqDisasm::Type::NONE && m_disasmLines >= DISASM_INSTRUCTION_OFFSET)
	{
		switch (m_reqDisasm.type)
		{
		case ReqDisasm::Type::UPDATE:
		{
			Addr addr = m_disasmP->at(DISASM_INSTRUCTION_OFFSET).addr;
			UpdateDisasm(addr, DISASM_INSTRUCTION_OFFSET, false);
		}
		break;

		case ReqDisasm::Type::UPDATE_ADDR:
			UpdateDisasm(m_reqDisasm.addr);
			break;

		case ReqDisasm::Type::NAVIGATE_TO_ADDR:
			if (m_navigateAddrsIdx == 0) {
				m_navigateAddrs[m_navigateAddrsIdx] = m_disasmP->at(DISASM_INSTRUCTION_OFFSET).addr;
				m_navigateAddrsSize++;
			}
			if (m_navigateAddrsIdx < NAVIGATE_ADDRS_LEN) {
				m_navigateAddrs[++m_navigateAddrsIdx] = m_reqDisasm.addr;
				m_navigateAddrsSize = m_navigateAddrsIdx + 1;
			}
			UpdateDisasm(m_reqDisasm.addr);
			break;

		default:
			break;
		}
		m_reqDisasm.type = ReqDisasm::Type::NONE;
	}

	if (_isRunning) ImGui::EndDisabled();
}


void dev::DisasmWindow::UpdateData(const bool _isRunning)
{
	if (_isRunning) return;

	auto res = m_hardware.Request(Hardware::Req::GET_REGS);
	const auto& data = *res;

	uint64_t cc = data["cc"];
	auto ccDiff = cc - m_ccLast;
	m_ccLastRun = ccDiff == 0 ? m_ccLastRun : ccDiff;
	m_ccLast = cc;
	if (ccDiff == 0) return;

	// update
	Addr addr = m_hardware.Request(Hardware::Req::GET_REG_PC)->at("pc");

	UpdateDisasm(addr);
}

void dev::DisasmWindow::UpdateDisasm(const Addr _addr, const int _instructionsOffset, const bool _updateSelection)
{
	m_disasmP = m_debugger.GetDisasm(_addr, m_disasmLines, -_instructionsOffset);
	if (_updateSelection) m_selectedLineIdx = DISASM_INSTRUCTION_OFFSET;
}

int dev::DisasmWindow::DrawDisasmContextMenu(const bool _openContextMenu, const Addr _regPC, 
		int _addr, int _copyToClipboardAddr, std::string& _str)
{
	if (_openContextMenu) {
		ImGui::OpenPopup(m_itemContextMenu);
	}

	if (ImGui::BeginPopup(m_itemContextMenu))
	{
		if (ImGui::MenuItem("Copy To Clipboard")) {
				dev::CopyToClipboard(_str);
			}
		if (_copyToClipboardAddr >= 0 ){
			if (ImGui::MenuItem("Copy Addr To Clipboard")) {
				dev::CopyToClipboard(std::format("0x{:04X}", _copyToClipboardAddr));
			}
		}
		ImGui::SeparatorText("");
		if (ImGui::MenuItem("Show Current Break")) {
			UpdateDisasm(_regPC);
		}
		ImGui::SeparatorText("");
		if (ImGui::MenuItem("Run To Selected Line") && _addr >= 0)
		{
			m_debugger.AddBreakpoint(_addr, Breakpoint::MAPPING_PAGES_ALL, Breakpoint::Status::ACTIVE, true);
			m_hardware.Request(Hardware::Req::RUN);
		}
		ImGui::SeparatorText("");
		if (ImGui::MenuItem("Add/Remove Beakpoint"))
		{
			auto bpStatus = m_debugger.GetBreakpointStatus(_addr);

			if (bpStatus == Breakpoint::Status::DELETED) {
				m_debugger.AddBreakpoint(_addr);
			}
			else {
				m_debugger.DelBreakpoint(_addr);
			}
			m_reqDisasm.type = dev::ReqDisasm::Type::UPDATE;
		}
		if (ImGui::MenuItem("Remove All Beakpoints")) {
			m_debugger.DelBreakpoints();
			m_reqDisasm.type = dev::ReqDisasm::Type::UPDATE;
		};
		ImGui::EndPopup();
	}

	return _copyToClipboardAddr;
}