#include <format>

#include "ui/disasm_window.h"
#include "utils/str_utils.h"


dev::DisasmWindow::DisasmWindow(
		dev::Hardware& _hardware, Debugger& _debugger, ImFont* fontComment,
		const float* const _dpiScale, 
		ReqUI& _reqUI)
	:
	BaseWindow("Disasm", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _dpiScale),
	m_hardware(_hardware),
	m_debugger(_debugger),
	m_fontCommentP(fontComment),
	m_reqUI(_reqUI)
{
	UpdateData(false);
}

void dev::DisasmWindow::Update(bool& _visible, const bool _isRunning)
{
	BaseWindow::Update();

	if (_visible && ImGui::Begin(m_name.c_str(), &_visible, ImGuiWindowFlags_NoCollapse))
	{
		DrawDebugControls(_isRunning);
		DrawSearch(_isRunning);


		bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning"); // in case it changed the bpStatus in DrawDebugControls
		UpdateData(isRunning);
		DrawDisasm(isRunning);

		ImGui::End();
	}

}

void dev::DisasmWindow::DrawDebugControls(const bool _isRunning)
{
	if (!_isRunning && ImGui::Button(" Run "))
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
	if (ImGui::Button("Step Over"))
	{
		Addr addr = m_hardware.Request(Hardware::Req::GET_STEP_OVER_ADDR)->at("data");
		Breakpoint::Data bpData
			{addr, Breakpoint::MAPPING_PAGES_ALL, Breakpoint::Status::ACTIVE, true};

		m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_ADD, {
			{"data0", bpData.data0 },
			{"data1", bpData.data1 },
			{"data2", bpData.data2 },
			{"comment", ""}
		});

		m_hardware.Request(Hardware::Req::RUN);
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
		m_ccLast = -1;
		m_reqUI.type = ReqUI::Type::RELOAD_ROM_FDD_REC;
		m_hardware.Request(Hardware::Req::STOP);
	}

	ImGui::SameLine();
	dev::DrawHelpMarker(
		"Break/Run stops and continues the execution.\n"
		"Step executes one command.\n"
		"Step Over executes the next command without entering it. For example, stepping over a Call, stops the progamm at the next instruction after Call.\n"
		"Step 0x100 executes 256 instructions.\n"
		"Step Frame executes until RST7 (the next frame start).\n"
		"Reset relaods the ROM/FDD file and reset the hardware keeping all brealpoints intact."
		);	
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
	float remainingSpace = ImGui::GetWindowSize().y - cursorPos.y - ImGui::GetFontSize();

	return remainingSpace < 0;
}

int dev::DisasmWindow::GetVisibleLines() const
{
	auto lines = (ImGui::GetWindowSize().y - ImGui::GetCursorPosY()) / ImGui::GetFontSize();
	return static_cast<int>(lines);
}

void dev::DisasmWindow::DrawDisasmIcons(const bool _isRunning, const Disasm::Line& _line, const int _lineIdx, const Addr _regPC)
{
	if (_isRunning) return;
	// draw breakpoints
	ImGui::SameLine();
	auto bpStatus = _line.breakpointStatus;
	if (dev::DrawBreakpoint(std::format("##BpAddr{:04d}", _lineIdx).c_str(), &bpStatus, *m_dpiScaleP))
	{
		switch (bpStatus)
		{
		case dev::Breakpoint::Status::DISABLED:
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DISABLE, { {"addr", _line.addr} });
			break;
		case dev::Breakpoint::Status::ACTIVE:
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_ACTIVE, { {"addr", _line.addr} });
			break;
		case dev::Breakpoint::Status::DELETED:
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DEL, { {"addr", _line.addr} });
			break;
		}
		m_reqUI.type = ReqUI::Type::DISASM_UPDATE;
	}

	// draw program counter icon
	if (_line.addr == _regPC)
	{
		ImGui::SameLine();
		dev::DrawProgramCounter(DASM_CLR_PC, ImGuiDir_Right, *m_dpiScaleP, PC_ICON_OFFSET_X);
	}
}

void dev::DisasmWindow::DrawDisasmAddr(const bool _isRunning, const Disasm::Line& _line, 
	ReqUI& _reqUI, ContextMenu& _contextMenu, AddrHighlight& _addrHighlight)
{
	// the addr column
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DASM_BG_CLR_ADDR);

	auto mouseAction = DrawAddr(_isRunning, _line.GetAddrS(),
		DASM_CLR_LABEL_MINOR, dev::IM_VEC4(0xFFFFFFFF), _addrHighlight.IsEnabled(_line.addr));
	switch (mouseAction)
	{
	case UIItemMouseAction::LEFT: // Navigate to the address
		_reqUI.type = ReqUI::Type::DISASM_NAVIGATE_TO_ADDR;
		_reqUI.globalAddr = _line.addr;
		break;
	case UIItemMouseAction::RIGHT:
		_contextMenu.Init(_line.addr, _line.GetAddrS());
		break;
	}
}

void dev::DisasmWindow::DrawDisasmCode(const bool _isRunning, const Disasm::Line& _line,
	ReqUI& _reqUI, ContextMenu& _contextMenu, AddrHighlight& _addrHighlight)
{
	// draw code
	ImGui::TableNextColumn();
	auto mouseAction = dev::DrawCodeLine(_isRunning, _line, true);
	// when a user did action to the immediate operand
	switch (mouseAction)
	{
	// any case below means that the immediate addr was at least hovered
	case UIItemMouseAction::LEFT: // Navigate to the address
		_reqUI.type = ReqUI::Type::DISASM_NAVIGATE_TO_ADDR;
		_reqUI.globalAddr = _line.imm;
		break;
	case UIItemMouseAction::RIGHT: // init the immediate value as an addr to let the context menu copy it
		_contextMenu.Init(_line.imm, _line.GetImmediateS(), true);
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

void dev::DisasmWindow::DrawDisasm(const bool _isRunning)
{
	if (!m_disasmPP || !*m_disasmPP) return;
	auto& disasm = **m_disasmPP;

	Addr regPC = m_hardware.Request(Hardware::Req::GET_REG_PC)->at("pc");
	int hoveredLineIdx = -1;
	ImVec2 selectionMin = ImGui::GetCursorScreenPos();
	ImVec2 selectionMax = ImVec2(selectionMin.x + ImGui::GetWindowWidth(), selectionMin.y);
	
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

		m_disasmLines = dev::Min((int)disasm.size(), GetVisibleLines());

		for (int lineIdx = 0; lineIdx < m_disasmLines; lineIdx++)
		{
			ImGui::TableNextRow();

			// the line selection/highlight
			ImGui::TableNextColumn();
			ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DIS_BG_CLR_BRK);
			const bool isSelected = m_selectedLineIdx == lineIdx;
			if (ImGui::Selectable(std::format("##disasm{}", lineIdx).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
			{
				m_selectedLineIdx = lineIdx;
			}
			if (!_isRunning) 
			{
				selectionMin.y = ImGui::GetItemRectMin().y;
				selectionMax.y = ImGui::GetItemRectMax().y;	
				hoveredLineIdx = ImGui::IsMouseHoveringRect(selectionMin, selectionMax) ? lineIdx : hoveredLineIdx;
			}

			auto& line = disasm.at(lineIdx);
			int addr = line.addr;

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
					DrawDisasmIcons(_isRunning, line, lineIdx, regPC);
					DrawDisasmAddr(_isRunning, line, m_reqUI, m_contextMenu, m_addrHighlight);
					DrawDisasmCode(_isRunning, line, m_reqUI, m_contextMenu, m_addrHighlight);

					DrawDisasmStats(line);
					DrawDisasmConsts(line, MAX_DISASM_LABELS);
					break;
				}
			}
			
			if (!_isRunning)
			{
				// line is hovered. check if right-clicked to open the Context menu
				if (hoveredLineIdx == lineIdx && m_contextMenu.status == ContextMenu::Status::NONE &&
					ImGui::IsMouseClicked(ImGuiMouseButton_Right)) 
				{
					m_contextMenu.Init(addr, line.GetStr());
				}
			}
		}

		ImGui::EndTable();
	}

	DrawContextMenu(regPC, m_contextMenu);
	DrawCommentEdit(m_contextMenu);
	DrawLabelEdit(m_contextMenu);
	DrawConstEdit(m_contextMenu);
	ImGui::PopStyleVar(2);

	/////////////////////////////////////////////////////////	
	// check the keys and the mouse
	/////////////////////////////////////////////////////////	
	if (!_isRunning && hoveredLineIdx >= 0)
	{
		// Up/Down scrolling
		if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
		{
			m_selectedLineIdx = dev::Min(m_selectedLineIdx + 2, m_disasmLines - 1);
			UpdateDisasm(disasm[0].addr, 2, false);
		}
		else if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
		{
			m_selectedLineIdx = dev::Max(m_selectedLineIdx - 2, 0);
			UpdateDisasm(disasm[0].addr, -2, false);
		}
		if (ImGui::GetIO().MouseWheel > 0.0f)
		{
			m_selectedLineIdx = dev::Min(m_selectedLineIdx + 2, m_disasmLines - 1);
			UpdateDisasm(disasm[0].addr, 2, false);
		}
		else if (ImGui::GetIO().MouseWheel < 0.0f)
		{
			m_selectedLineIdx = dev::Max(m_selectedLineIdx - 2, 0);
			UpdateDisasm(disasm[0].addr, -2, false);
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
	}

	if (_isRunning) ImGui::EndDisabled();
}


void dev::DisasmWindow::UpdateData(const bool _isRunning)
{
	ReqHandling(); // should be before UpdateDisasm and DrawDisasm 

	if (_isRunning) return;

	auto res = m_hardware.Request(Hardware::Req::GET_CC);
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

void dev::DisasmWindow::ReqHandling()
{
	if (m_reqUI.type == ReqUI::Type::NONE) return;

	switch (m_reqUI.type)
	{
	case ReqUI::Type::DISASM_UPDATE:
	{
		m_reqUI.type = ReqUI::Type::NONE;
		UpdateDisasm(m_disasmAddr, DISASM_INSTRUCTION_OFFSET, false);
	}
	break;

	case ReqUI::Type::DISASM_UPDATE_ADDR:
		m_reqUI.type = ReqUI::Type::NONE;
		UpdateDisasm(m_reqUI.globalAddr);
		break;

	case ReqUI::Type::DISASM_NAVIGATE_TO_ADDR:
		m_reqUI.type = ReqUI::Type::NONE;
		if (m_navigateAddrsIdx == 0) 
		{
			if (!m_disasmPP || !*m_disasmPP) return;
			m_navigateAddrs[m_navigateAddrsIdx] = (*m_disasmPP)->at(DISASM_INSTRUCTION_OFFSET).addr;
			m_navigateAddrsSize++;
		}
		if (m_navigateAddrsIdx < NAVIGATE_ADDRS_LEN) {
			m_navigateAddrs[++m_navigateAddrsIdx] = m_reqUI.globalAddr;
			m_navigateAddrsSize = m_navigateAddrsIdx + 1;
		}
		UpdateDisasm(m_reqUI.globalAddr);
		break;
	}
}

void dev::DisasmWindow::UpdateDisasm(const Addr _addr, const int _instructionsOffset, const bool _updateSelection)
{
	m_disasmAddr = _addr;
	m_debugger.UpdateDisasm(_addr, m_disasmLines, -_instructionsOffset);
	m_disasmPP = m_debugger.GetDisasm().GetLines();
	m_immLinksP = m_debugger.GetDisasm().GetImmLinks();
	m_immLinksNum = m_debugger.GetDisasm().GetImmAddrlinkNum();

	if (_updateSelection) m_selectedLineIdx = DISASM_INSTRUCTION_OFFSET;
}

void dev::DisasmWindow::DrawContextMenu(const Addr _regPC, ContextMenu& _contextMenu)
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
		if (ImGui::MenuItem("Show Current Break")) {
			UpdateDisasm(_regPC);
		}
		ImGui::SeparatorText("");
		if (ImGui::MenuItem("Run To"))
		{
			Breakpoint::Data bpData{ 
				_contextMenu.addr, Breakpoint::MAPPING_PAGES_ALL, Breakpoint::Status::ACTIVE, true };
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
			Breakpoint::Status bpStatus = static_cast<Breakpoint::Status>(m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_GET_STATUS, { {"addr"} })->at("status"));

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
			m_reqUI.type = dev::ReqUI::Type::DISASM_UPDATE;
		}
		if (ImGui::MenuItem("Remove All Beakpoints")) {
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DEL_ALL);
			m_reqUI.type = dev::ReqUI::Type::DISASM_UPDATE;
		};
		ImGui::SeparatorText("");
		if (ImGui::MenuItem("Add/Edit Comment")) {
			_contextMenu.status = ContextMenu::Status::INIT_COMMENT_EDIT;
		};
		if (ImGui::MenuItem("Add/Edit Label")) {
			_contextMenu.status = ContextMenu::Status::INIT_LABEL_EDIT;
		};
		if (_contextMenu.immHovered && ImGui::MenuItem("Add/Edit Const")) {
			_contextMenu.status = ContextMenu::Status::INIT_CONST_EDIT;
		};
		ImGui::EndPopup();
	}
}

void dev::DisasmWindow::DrawCommentEdit(ContextMenu& _contextMenu)
{
	static ImVec2 buttonSize = { 65.0f, 25.0f };
	static ImVec2 buttonSizeX = { 45.0f, 25.0f };
	static char comment[255] = "";
	bool enterPressed = false;
	bool selectText = false;

	if (_contextMenu.status == ContextMenu::Status::INIT_COMMENT_EDIT)
	{
		ImGui::OpenPopup(_contextMenu.commentEditName);
		_contextMenu.status = ContextMenu::Status::NONE;
		auto currentComment = m_debugger.GetDebugData().GetComment(_contextMenu.addr);
		if (currentComment) {
			snprintf(comment, sizeof(comment), currentComment->c_str());
		}
		else {
			comment[0] = '\0';
		}
		selectText = true;
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter(); 	// Always center this window
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(_contextMenu.commentEditName, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// edit comment
		if (selectText) ImGui::SetKeyboardFocusHere();
		if (ImGui::InputTextWithHint("##comment", "", comment, IM_ARRAYSIZE(comment), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
			enterPressed = true;
		}
		
		// Delete button
		ImGui::SameLine(); ImGui::Dummy(UI_LITTLE_SPACE); ImGui::SameLine();
		if (ImGui::Button("X", buttonSizeX))
		{
			comment[0] = '\0';
		}

		ImGui::SameLine(); ImGui::Dummy(UI_LITTLE_SPACE); ImGui::SameLine();
		dev::DrawHelpMarker("A semicolon is not required.");

		ImGui::SeparatorText("");

		// property table
		static ImGuiTableFlags flags =
			ImGuiTableFlags_SizingStretchSame |
			ImGuiTableFlags_ContextMenuInBody;

		if (ImGui::BeginTable("##CETable", 2, flags))
		{
			ImGui::TableSetupColumn("##CEContextMenuName", ImGuiTableColumnFlags_WidthFixed, 150);
			ImGui::TableSetupColumn("##CEContextMenuVal", ImGuiTableColumnFlags_WidthFixed, 200);		

			// warnings
			std::string warningS = "";

			// OK/CANCEL/... butons
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			// OK button
			if (!warningS.empty()) ImGui::BeginDisabled();
			if (ImGui::Button("Ok", buttonSize) || enterPressed)
			{
				if (comment[0]){ // non-empty string
					m_debugger.GetDebugData().SetComment(_contextMenu.addr, comment);
				}
				else {
					m_debugger.GetDebugData().DelComment(_contextMenu.addr);
				}

				m_reqUI.type = dev::ReqUI::Type::DISASM_UPDATE;
				ImGui::CloseCurrentPopup();
			}
			if (!warningS.empty()) ImGui::EndDisabled();

			// Cancel button
			ImGui::SameLine(); ImGui::Text(" "); ImGui::SameLine();
			if (ImGui::Button("Cancel", buttonSize)) ImGui::CloseCurrentPopup();

			// ESC pressed
			if (ImGui::IsKeyReleased(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();

			ImGui::EndTable();
		}

		ImGui::EndPopup();
	}
}

void DeleteByIndex(dev::Disasm::LabelList& _labels, char* _label, int& _idx)
{
	if (_labels.size() > 1) {
		_labels.erase(_labels.begin() + _idx);
	}
	else {
		_labels[_idx].clear();
	}
	_idx = 0;
	snprintf(_label, sizeof(_label), _labels.at(_idx).c_str());
}

void dev::DisasmWindow::DrawConstEdit(ContextMenu& _contextMenu)
{
	static ImVec2 buttonSize = { 65.0f, 25.0f };
	static ImVec2 buttonSizeX = { 45.0f, 25.0f };
	static char const_[255] = "";
	static int editedConstIdx = 0;
	bool enterPressed = false;
	bool selectText = false;
	auto constsP = m_debugger.GetDebugData().GetConsts(_contextMenu.addr);
	static Disasm::LabelList consts;

	if (_contextMenu.status == ContextMenu::Status::INIT_CONST_EDIT)
	{
		ImGui::OpenPopup(_contextMenu.constEditName);
		_contextMenu.status = ContextMenu::Status::NONE;
		selectText = true;
		editedConstIdx = 0;
		consts.clear();

		if (constsP) {
			snprintf(const_, sizeof(const_), constsP->at(editedConstIdx).c_str());
			for (const auto& str : *constsP) {
				consts.push_back(str);
			}
		}
		else {
			const_[0] = '\0';
			consts.push_back("");
		}
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter(); 	// Always center this window
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(_contextMenu.constEditName, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// edit comment
		if (selectText) ImGui::SetKeyboardFocusHere();
		if (ImGui::InputTextWithHint("##const", "", const_, IM_ARRAYSIZE(const_), 
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsUppercase)) {
			enterPressed = true;
		}
		consts[editedConstIdx] = const_;

		// Delete button
		ImGui::SameLine(); ImGui::Dummy(UI_LITTLE_SPACE); ImGui::SameLine();
		if (ImGui::Button("X", buttonSizeX))
		{
			DeleteByIndex(consts, const_, editedConstIdx);
		}
		ImGui::SeparatorText("");

		// list all consts
		if (constsP)
		{
			if (ImGui::BeginListBox("##CListBox"))
			{
				auto constsNum = consts.size();
				for (int constIdx = 0; constIdx < constsNum; constIdx++)
				{
					auto& str = consts.at(constIdx);
					const bool is_selected = (editedConstIdx == constIdx);
					if (ImGui::Selectable(std::format("{}##{}", str, constIdx).c_str(), is_selected))
					{
						consts[editedConstIdx] = const_;
						editedConstIdx = constIdx;
						snprintf(const_, sizeof(const_), consts.at(editedConstIdx).c_str());

					}

					if (is_selected) {// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}
			ImGui::SameLine(); ImGui::Dummy(UI_LITTLE_SPACE); ImGui::SameLine();
			dev::DrawHelpMarker("This list contains all consts with the same value.\n"
				"Specify which const fits this context best.");
			ImGui::SeparatorText("");
		}

		// property table
		static ImGuiTableFlags flags =
			ImGuiTableFlags_SizingStretchSame |
			ImGuiTableFlags_ContextMenuInBody;

		if (ImGui::BeginTable("##CETable", 2, flags))
		{
			ImGui::TableSetupColumn("##CEContextMenuName", ImGuiTableColumnFlags_WidthFixed, 150);
			ImGui::TableSetupColumn("##CEContextMenuVal", ImGuiTableColumnFlags_WidthFixed, 200);

			// warnings
			std::string warningS = "";

			// OK/CANCEL/... butons
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			// OK button
			if (!warningS.empty()) ImGui::BeginDisabled();
			if (ImGui::Button("Ok", buttonSize) || enterPressed)
			{
				// remove empty consts
				consts.erase(std::remove_if(consts.begin(), consts.end(),
					[](const std::string& _const) { return _const.empty(); }), consts.end());
				// store consts
				m_debugger.GetDebugData().SetConsts(_contextMenu.addr, consts);
				m_reqUI.type = dev::ReqUI::Type::DISASM_UPDATE;
				ImGui::CloseCurrentPopup();
			}
			if (!warningS.empty()) ImGui::EndDisabled();

			// Cancel button
			ImGui::SameLine(); ImGui::Text(" "); ImGui::SameLine();
			if (ImGui::Button("Cancel", buttonSize)) ImGui::CloseCurrentPopup();

			// ESC pressed
			if (ImGui::IsKeyReleased(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();

			ImGui::EndTable();
		}

		ImGui::EndPopup();
	}
}

void dev::DisasmWindow::DrawLabelEdit(ContextMenu& _contextMenu)
{
	static ImVec2 buttonSize = { 65.0f, 25.0f };
	static ImVec2 buttonSizeX = { 45.0f, 25.0f };
	static char label[255] = "";
	static int editedLabelIdx = 0;
	bool enterPressed = false;
	bool selectText = false;
	auto labelsP = m_debugger.GetDebugData().GetLabels(_contextMenu.addr);
	static Disasm::LabelList labels;

	if (_contextMenu.status == ContextMenu::Status::INIT_LABEL_EDIT)
	{
		ImGui::OpenPopup(_contextMenu.labelEditName);
		_contextMenu.status = ContextMenu::Status::NONE;
		selectText = true;
		editedLabelIdx = 0;
		labels.clear();

		if (labelsP) {
			snprintf(label, sizeof(label), labelsP->at(editedLabelIdx).c_str());
			for (const auto& str : *labelsP) {
				labels.push_back(str);
			}
		}
		else {
			label[0] = '\0';
			labels.push_back("");
		}
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter(); 	// Always center this window
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(_contextMenu.labelEditName, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// edit comment
		if (selectText) ImGui::SetKeyboardFocusHere();
		if (ImGui::InputTextWithHint("##label", "", label, IM_ARRAYSIZE(label), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
			enterPressed = true;
		}
		labels[editedLabelIdx] = label;

		// Delete button
		ImGui::SameLine(); ImGui::Dummy(UI_LITTLE_SPACE); ImGui::SameLine();
		if (ImGui::Button("X", buttonSizeX))
		{
			DeleteByIndex(labels, label, editedLabelIdx);
		}

		ImGui::SameLine(); ImGui::Dummy(UI_LITTLE_SPACE); ImGui::SameLine();
		dev::DrawHelpMarker("A colon is not required.");
		ImGui::SeparatorText("");

		// list all labels
		if (labelsP)
		{	
			if (ImGui::BeginListBox("##lListBox"))
			{
				auto labelsNum = labels.size();
				for (int labelIdx = 0; labelIdx < labelsNum; labelIdx++)
				{
					auto& str = labels.at(labelIdx);
					const bool is_selected = (editedLabelIdx == labelIdx);
					if (ImGui::Selectable(std::format("{}##{}", str, labelIdx).c_str(), is_selected))
					{
						labels[editedLabelIdx] = label;
						editedLabelIdx = labelIdx;
						snprintf(label, sizeof(label), labels.at(editedLabelIdx).c_str());

					}

					if (is_selected) {// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}
			ImGui::SameLine(); ImGui::Dummy(UI_LITTLE_SPACE); ImGui::SameLine();
			dev::DrawHelpMarker("This list contains all labels for this address.\n"
				"Specify which one fits this context best.");
			ImGui::SeparatorText("");
		}

		// property table
		static ImGuiTableFlags flags =
			ImGuiTableFlags_SizingStretchSame |
			ImGuiTableFlags_ContextMenuInBody;

		if (ImGui::BeginTable("##CETable", 2, flags))
		{
			ImGui::TableSetupColumn("##CEContextMenuName", ImGuiTableColumnFlags_WidthFixed, 150);
			ImGui::TableSetupColumn("##CEContextMenuVal", ImGuiTableColumnFlags_WidthFixed, 200);

			// warnings
			std::string warningS = "";

			// OK/CANCEL/... butons
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			// OK button
			if (!warningS.empty()) ImGui::BeginDisabled();
			if (ImGui::Button("Ok", buttonSize) || enterPressed)
			{
				// remove empty labels
				labels.erase(std::remove_if(labels.begin(), labels.end(),
					[](const std::string& label) { return label.empty(); }), labels.end()); 
				// store labels
				m_debugger.GetDebugData().SetLabels(_contextMenu.addr, labels);
				m_reqUI.type = dev::ReqUI::Type::DISASM_UPDATE;
				ImGui::CloseCurrentPopup();
			}
			if (!warningS.empty()) ImGui::EndDisabled();

			// Cancel button
			ImGui::SameLine(); ImGui::Text(" "); ImGui::SameLine();
			if (ImGui::Button("Cancel", buttonSize)) ImGui::CloseCurrentPopup();

			// ESC pressed
			if (ImGui::IsKeyReleased(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();

			ImGui::EndTable();
		}

		ImGui::EndPopup();
	}
}

void dev::DisasmWindow::DrawAddrLinks(const bool _isRunning, const int _lineIdx, 
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

	bool minorLink = (link.lineIdx == Disasm::IMM_LINK_UP || link.lineIdx == Disasm::IMM_LINK_DOWN || link.lineIdx == Disasm::IMM_NO_LINK);
	ImU32 linkColor = _selected ? DIS_CLR_LINK_HIGHLIGHT : DIS_CLR_LINK;
	
	if (minorLink)
	{	// links to the addrs outside the disasm view
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
		auto pos2 = ImVec2(pos1.x, pos1.y + (link.lineIdx - _lineIdx) * ImGui::GetFontSize());
		ImGui::GetForegroundDrawList()->AddLine(pos1, pos2, linkColor, linkWidth);
		// horizontal line to the addr
		pos0.y = pos2.y;
		ImGui::GetForegroundDrawList()->AddLine(pos2, pos0, linkColor, linkWidth);
		// arrow
		float r = 5.0f;
		ImVec2 center = pos0;
		auto a = ImVec2(center.x + 0.750f * r, center.y);
		auto b = ImVec2(center.x - 0.750f * r, center.y + 0.866f * r);
		auto c = ImVec2(center.x - 0.750f * r, center.y - 0.866f * r);
		ImGui::GetForegroundDrawList()->AddTriangleFilled(a, b, c, linkColor);
	}
}