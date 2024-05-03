#include <format>
#include "DisasmWindow.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/StringUtils.h"


dev::DisasmWindow::DisasmWindow(
		dev::Hardware& _hardware, Debugger& _debugger, ImFont* fontComment,
		const float* const _fontSize, const float* const _dpiScale, ReqDisasm& _reqDisasm, bool& _reset)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSize, _dpiScale),
	m_hardware(_hardware),
	m_debugger(_debugger),
	m_fontCommentP(fontComment),
	m_reqDisasm(_reqDisasm),
	m_reset(_reset), m_navigateAddrs()
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
		m_reset = true;
		m_hardware.Request(Hardware::Req::STOP);
		m_hardware.Request(Hardware::Req::RESET);
		m_debugger.ReqLoadRomLast();
		m_debugger.Reset();
		m_hardware.Request(Hardware::Req::RUN);
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
	float remainingSpace = ImGui::GetWindowSize().y - cursorPos.y - *m_fontSizeP * 1.0f;

	return remainingSpace < 0;
}

void dev::DisasmWindow::DrawDisasm(const bool _isRunning)
{
	auto res = m_hardware.Request(Hardware::Req::GET_REGS);
	const auto& regs = *res;
	Addr regPC = regs["pc"];
	bool openItemContextMenu = false;
	static int editedBreakpointAddr = -1;
	int reqUpdateAddr = -1;
	static int copyToClipboardAddr = -1; // if it's -1, don't add the option, if it's >=0, add the option with the addr = copyToClipboardAddr
	static int delayedSelectionTimer = -1;
	static int delayedSelectionLine = 0;

	if (m_disasm.empty()) return;

	if (_isRunning) ImGui::BeginDisabled();

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5, 0 });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	int lineIdx = 0;

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

		for (; lineIdx < 200; lineIdx++)
			{
				// TODO: fix it. replace with fixed amount of lines and without a need to check the end of the window
				if (IsDisasmTableOutOfWindow()) break;

				ImGui::TableNextRow();

				if (lineIdx >= m_disasm.size()) break;

				auto& line = m_disasm[lineIdx];

				bool isComment = line.type == Debugger::DisasmLine::Type::COMMENT;
				bool isCode = line.type == Debugger::DisasmLine::Type::CODE;
				int addr = line.addr;

				// the line selection/highlight
				ImGui::TableNextColumn();
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DISASM_TBL_BG_COLOR_BRK);
				const bool isSelected = m_selectedLineIdx == lineIdx;
				if (ImGui::Selectable(std::format("##disasmLineId{:04d}", lineIdx).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
				{
					m_selectedLineIdx = lineIdx;
				}

				if (!_isRunning) {
					// draw breakpoints
					ImGui::SameLine();
					auto bpStatus = line.breakpointStatus;
					if (dev::DrawBreakpoint(std::format("##BpAddr{:04d}", lineIdx).c_str(), &bpStatus, *m_dpiScaleP))
					{
						if (bpStatus == Breakpoint::Status::DELETED) {
							m_debugger.DelBreakpoint(addr);
						}
						else m_debugger.SetBreakpointStatus(addr, bpStatus);
						m_reqDisasm.type = ReqDisasm::Type::UPDATE;
					}

					// draw program counter icon
					if (isCode && addr == regPC)
					{
						ImGui::SameLine();
						dev::DrawProgramCounter(DISASM_TBL_COLOR_PC, ImGuiDir_Right, *m_dpiScaleP);
					}
				}

				// the addr column
				ImGui::TableNextColumn();
				ColumnClippingEnable(*m_dpiScaleP); // enable clipping
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DISASM_TBL_BG_COLOR_ADDR);
				if (isCode) {
					DrawAddr(_isRunning, line, 
						// _onMouseLeft. Navigate to the address
						[&]()
						{},
						// _onMouseRight. Add the "Copy to Clipboard" option to the context menu
						[&]()
						{
							copyToClipboardAddr = addr;
						}
					);
				}
				ColumnClippingDisable();

				ImVec2 rowMin = ImGui::GetItemRectMin();
				ImVec2 rowMax = { -1.0f, -1.0f };


				// draw a comment
				if (isComment)
				{
					if (m_fontCommentP) { ImGui::PushFont(m_fontCommentP);}
					ImGui::TableNextColumn();
					ImGui::TextColored(DISASM_TBL_COLOR_COMMENT, line.str.c_str());

					if (m_fontCommentP) { ImGui::PopFont(); }
				}

				// draw labels
				else if (!isCode)
				{
					auto line_splited = dev::Split(line.str, '\t');
					ImGui::TableNextColumn();
					int i = 0;
					for (auto const& label : line_splited)
					{
						// the label that matches the address and the code context
						if (i == 0)
						{
							if (label[0] == '@')
							{
								ImGui::TextColored(DISASM_TBL_COLOR_LABEL_LOCAL, label.c_str());
							}
							else
							{
								ImGui::TextColored(DISASM_TBL_COLOR_LABEL_GLOBAL, label.c_str());
							}
						}
						// all other labels that matches the address
						else
						{
							ImGui::SameLine();
							ImGui::TextColored(DISASM_TBL_COLOR_LABEL_MINOR, " %s", label.c_str());
						}
						i++;
					}
				}
				else
				{
					// draw code
					ImGui::TableNextColumn();
					ColumnClippingEnable(*m_dpiScaleP); // enable clipping

					dev::DrawCodeLine(true, _isRunning, line, 
						// _onMouseLeft. Navigate to the address
						[&](const Addr _addr)
						{
							reqUpdateAddr = _addr;

							if (m_navigateAddrsIdx == 0) {
								m_navigateAddrs[m_navigateAddrsIdx] = line.addr;
								m_navigateAddrsSize++;
							}
							if (m_navigateAddrsIdx < NAVIGATE_ADDRS_LEN) {
								m_navigateAddrs[++m_navigateAddrsIdx] = reqUpdateAddr;
								m_navigateAddrsSize = m_navigateAddrsIdx + 1;
							}

							// move selection to the DISASM_INSTRUCTION_OFFSET'th line
							delayedSelectionTimer = DELAYED_SELECTION_TIME;
							delayedSelectionLine = DISASM_INSTRUCTION_OFFSET;
						},
						// _onMouseRight. Add the "Copy to Clipboard" option to the context menu
						[&](const Addr _addr)
						{
							copyToClipboardAddr = _addr;
						}
					);

					ColumnClippingDisable();

					// draw stats
					ImGui::TableNextColumn();
					ColumnClippingEnable(*m_dpiScaleP); // enable clipping
					ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(DISASM_TBL_BG_COLOR_ADDR));
					const ImVec4* statsColor = (line.runs == 0 && line.reads == 0 && line.writes == 0) ?
							&DISASM_TBL_COLOR_ZERO_STATS :
							&DISASM_TBL_COLOR_ADDR;
					ImGui::TextColored(*statsColor, line.stats.c_str());
					ColumnClippingDisable();

					// draw consts
					ImGui::TableNextColumn();
					if (isCode)
					{
						ImGui::TextColored(DISASM_TBL_COLOR_ADDR, line.consts.c_str());
					}

					rowMax = ImGui::GetItemRectMax();
				}

				// check if right-click on the row
				if (!_isRunning && rowMax.x != -1.0f && rowMax.y != -1.0f &&
					ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					auto mousePos = ImGui::GetMousePos();
					if (mousePos.x >= rowMin.x && mousePos.x < rowMax.x &&
						mousePos.y >= rowMin.y && mousePos.y < rowMax.y)
					{
						openItemContextMenu = true;
						editedBreakpointAddr = addr;
					}
				}
			}

		ImGui::EndTable();
	}
	DrawDisasmContextMenu(openItemContextMenu, regPC, editedBreakpointAddr, copyToClipboardAddr);

	ImGui::PopStyleVar(2);

	if (!_isRunning && m_reqDisasm.type != ReqDisasm::Type::NONE && m_disasm.size() >= DISASM_INSTRUCTION_OFFSET)
	{
		Addr addr = 0;
		if (m_reqDisasm.type == ReqDisasm::Type::UPDATE)
		{
			addr = m_disasm[DISASM_INSTRUCTION_OFFSET].addr;
		}
		else if (m_reqDisasm.type == ReqDisasm::Type::UPDATE_ADDR){
			addr = m_reqDisasm.addr;

		}
		UpdateDisasm(addr);
		m_reqDisasm.type = ReqDisasm::Type::NONE;
	}

	// check the keys
	if (!_isRunning && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
		m_disasm.size() >= DISASM_INSTRUCTION_OFFSET)
	{
		// TODO: BUG: the code below only handles up and down arrow keys once for some reason
		/*
		if (ImGui::IsKeyDown(ImGuiKey_PageUp))
		{
			m_selectedLineIdx = dev::Min(m_selectedLineIdx + 1, lineIdx - 1);
			UpdateDisasm(m_disasm[DISASM_INSTRUCTION_OFFSET].addr, -1 - DISASM_INSTRUCTION_OFFSET);
		}
		else if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
		{
			{
				m_selectedLineIdx += 1;
				if (m_selectedLineIdx > lineIdx - 1) {
					m_selectedLineIdx = lineIdx - 1;
					UpdateDisasm(m_disasm[0].addr, 1);
				}
			}
		}
		*/
		if (ImGui::GetIO().MouseWheel > 0.0f)
		{
			m_selectedLineIdx = dev::Min(m_selectedLineIdx + 2, lineIdx - 1);
			UpdateDisasm(m_disasm[DISASM_INSTRUCTION_OFFSET].addr, -2 - DISASM_INSTRUCTION_OFFSET);
		}
		else if (ImGui::GetIO().MouseWheel < 0.0f)
		{
			m_selectedLineIdx = dev::Max(m_selectedLineIdx - 2, 0);
			UpdateDisasm(m_disasm[DISASM_INSTRUCTION_OFFSET].addr, 2 - DISASM_INSTRUCTION_OFFSET);
		}
	}

	// Alt + Left navigation
	if (ImGui::IsKeyDown(ImGuiKey_LeftAlt) && ImGui::IsKeyPressed(ImGuiKey_LeftArrow) &&
		m_navigateAddrsIdx - 1 >= 0)
	{
		auto addr = m_navigateAddrs[--m_navigateAddrsIdx];
		UpdateDisasm(addr);

		// move selection to the DISASM_INSTRUCTION_OFFSET'th line
		delayedSelectionTimer = DELAYED_SELECTION_TIME;
		delayedSelectionLine = DISASM_INSTRUCTION_OFFSET;
	}
	// Alt + Right navigation
	else if (ImGui::IsKeyDown(ImGuiKey_LeftAlt) && ImGui::IsKeyPressed(ImGuiKey_RightArrow) &&
		m_navigateAddrsIdx + 1 < m_navigateAddrsSize)
	{
		auto addr = m_navigateAddrs[++m_navigateAddrsIdx];
		UpdateDisasm(addr);

		// move selection to the DISASM_INSTRUCTION_OFFSET'th line
		delayedSelectionTimer = DELAYED_SELECTION_TIME;
		delayedSelectionLine = DISASM_INSTRUCTION_OFFSET;
	}

	// handle the delayed line selection
	delayedSelectionTimer -= delayedSelectionTimer >= 0 ? 1 : 0;
	if (delayedSelectionTimer == 0) {
		m_selectedLineIdx = DISASM_INSTRUCTION_OFFSET;
	}

	// update the disasm if the addr was clicked
	if (reqUpdateAddr >= 0) {
		UpdateDisasm(reqUpdateAddr);
	}

	if (_isRunning) ImGui::EndDisabled();
}


void dev::DisasmWindow::UpdateData(const bool _isRunning, int64_t _addr, const int _instructionsOffset)
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
	if (_addr < 0) {
		_addr = m_hardware.Request(Hardware::Req::GET_REG_PC)->at("pc");
	}

	m_selectedLineIdx = -_instructionsOffset;
	UpdateDisasm((Addr)_addr, _instructionsOffset);
}

void dev::DisasmWindow::UpdateDisasm(const Addr _addr, const int _instructionsOffset)
{
	// TODO: request a meaningful amount disasmm lines, not 80!
	m_disasm = m_debugger.GetDisasm(_addr, 80, _instructionsOffset);
}

int dev::DisasmWindow::DrawDisasmContextMenu(const bool _openContextMenu, const Addr _regPC, int _addr, int _copyToClipboardAddr)
{
	if (_openContextMenu) ImGui::OpenPopup("DisasmItemMenu");

	if (ImGui::BeginPopup("DisasmItemMenu"))
	{
		if (_copyToClipboardAddr >= 0 ){
			if (ImGui::MenuItem("Copy To Clipboard")) {
				dev::CopyToClipboard(std::format("0x{:04X}", _copyToClipboardAddr));
				_copyToClipboardAddr = -1;
			}
			ImGui::SeparatorText("");
		}
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