#include <format>
#include "DisasmWindow.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/StringUtils.h"


dev::DisasmWindow::DisasmWindow(
        dev::Hardware& _hardware, Debugger& _debugger, ImFont* fontComment,
        const float* const _fontSize, const float* const _dpiScale, bool& _reqDisasmUpdate)
    :
    BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSize, _dpiScale),
    m_hardware(_hardware),
    m_debugger(_debugger),
    m_fontCommentP(fontComment),
    m_reqDisasmUpdate(_reqDisasmUpdate)
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
    
    isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning"); // in case it changed the status in DrawDebugControls
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
        m_hardware.Request(Hardware::Req::EXECUTE_FRAME);
    }

    if (_isRunning) ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Reset"))
    {
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
    if (ImGui::InputTextWithHint("##empty 1", "0x100", m_searchText, IM_ARRAYSIZE(m_searchText))) 
    {
        Addr addr = (Addr)dev::StrHexToInt(m_searchText);
        UpdateDisasm(addr);
        m_selectedLineIdx = DISASM_INSTRUCTION_OFFSET;
    }
    ImGui::SameLine(); dev::DrawHelpMarker(
        "Search by a hexadecimal address in the format of 0x100 or 100,\n"
        "or by a case-sensitive label name.");
    ImGui::PopItemWidth();
    if (_isRunning) ImGui::EndDisabled();
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
    const auto& data = *res;
    Addr regPC = data["pc"];

    if (m_disasm.empty()) return;

    if (_isRunning) ImGui::BeginDisabled();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        
    int lineIdx = 0;

    PushStyleCompact();

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

                // draw breakpoints
                ImGui::SameLine();
                auto bpStatus = line.breakpointStatus;
                if (dev::DrawBreakpoint(std::format("##BpAddr{:04d}", lineIdx).c_str(), &bpStatus, *m_dpiScaleP))
                {
                    m_debugger.SetBreakpointStatus(addr, bpStatus);
                    m_reqDisasmUpdate = true;
                }

                // draw program counter icon
                if (isCode && addr == regPC && !_isRunning)
                {
                    ImGui::SameLine();
                    dev::DrawProgramCounter(DISASM_TBL_COLOR_PC, ImGuiDir_Right, *m_dpiScaleP);
                }



                // the addr column
                ImGui::TableNextColumn();
                ColumnClippingEnable(*m_dpiScaleP); // enable clipping
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DISASM_TBL_BG_COLOR_ADDR);
                if (isCode) { 
                    ImGui::TextColored(DISASM_TBL_COLOR_LABEL_MINOR, line.addrS.c_str());
                }
                ColumnClippingDisable();


                // draw a comment
                if (isComment)
                {
                    if (m_fontCommentP) {
                        ImGui::PushFont(m_fontCommentP);
                    }

                    // the code column
                    ImGui::TableNextColumn();
                    ImGui::TextColored(DISASM_TBL_COLOR_COMMENT, line.str.c_str());

                    // Revert to the default font
                    if (m_fontCommentP) {
                        ImGui::PopFont();
                    }

                }
                // draw labels
                else if (!isCode)
                {
                    auto line_splited = dev::Split(line.str, '\t');
                    // the code column
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
                // draw a code, stats, consts cells
                else
                {
                    ImGui::TableNextColumn();
                    ColumnClippingEnable(*m_dpiScaleP); // enable clipping
                    auto cmd_splitted = dev::Split(line.str, ' ');
                    int i = 0;
                    for (const auto& cmd_parts : cmd_splitted)
                    {
                        if (i == 0)
                        {
                            // draw a mnenonic
                            ImGui::TextColored(DISASM_TBL_COLOR_MNEMONIC, "\t%s ", cmd_parts.c_str());
                        }
                        else
                        {
                            // draw an operand separator
                            if (i == 2)
                            {
                                ImGui::SameLine();
                                ImGui::TextColored(DISASM_TBL_COLOR_NUMBER, ", ");
                            }

                            // draw an operand
                            auto operands = dev::Split(cmd_parts, '=');

                            for (const auto& operand : operands)
                            {
                                if (operand[0] == '0')
                                {
                                    // draw a hexadecimal literal
                                    ImGui::SameLine();
                                    ImGui::TextColored(DISASM_TBL_COLOR_NUMBER, "%s", operand.c_str());
                                }
                                else if (cmd_parts.size() <= 2)
                                {
                                    // draw a reg
                                    ImGui::SameLine();
                                    ImGui::TextColored(DISASM_TBL_COLOR_REG, "%s", operand.c_str());
                                }
                                else
                                {
                                    // draw a const
                                    ImGui::SameLine();
                                    ImGui::TextColored(DISASM_TBL_COLOR_CONST, "%s = ", operand.c_str());
                                }

                            }
                        }
                        i++;
                    }
                    ColumnClippingDisable();

                    // the stats column
                    ImGui::TableNextColumn();
                    ColumnClippingEnable(*m_dpiScaleP); // enable clipping
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(DISASM_TBL_BG_COLOR_ADDR));
                    const ImVec4* statsColor = (line.runs == 0 && line.reads == 0 && line.writes == 0) ? &DISASM_TBL_COLOR_ZERO_STATS : &DISASM_TBL_COLOR_ADDR;
                    ImGui::TextColored(*statsColor, line.stats.c_str());
                    ColumnClippingDisable();

                    // the consts column
                    ImGui::TableNextColumn();
                    if (isCode)
                    {
                        ImGui::TextColored(DISASM_TBL_COLOR_ADDR, line.consts.c_str());
                    }

                }
            }
       
        ImGui::EndTable();
        PopStyleCompact();
    }

    ImGui::PopStyleVar(2);

    if (m_reqDisasmUpdate && m_disasm.size() >= 1)
    {
        UpdateDisasm(m_disasm[0].addr, 0);
        m_reqDisasmUpdate = false;
    }

    // check the keys
    if (!_isRunning && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
        {
            if (m_disasm.size() >= 1)
            {
                m_selectedLineIdx--;
                if (m_selectedLineIdx < 0) {
                    m_selectedLineIdx = 0;
                    UpdateDisasm(m_disasm[0].addr, -1);
                }
            }
        }
        else if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
        {
            if (m_disasm.size() >= 1)
            {
                m_selectedLineIdx += 1;
                if (m_selectedLineIdx > lineIdx - 1) {
                    m_selectedLineIdx = lineIdx - 1;
                    UpdateDisasm(m_disasm[0].addr, 1);
                }
            }
        }

        if (ImGui::GetIO().MouseWheel > 0.0f)
        {
            if (m_disasm.size() >= 1)
            {
                m_selectedLineIdx--;
                if (m_selectedLineIdx < 0) {
                    m_selectedLineIdx = 0;
                    UpdateDisasm(m_disasm[0].addr, -2);
                }
            }
        }
        else if (ImGui::GetIO().MouseWheel < 0.0f)
        {
            if (m_disasm.size() >= 1)
            {
                m_selectedLineIdx += 1;
                if (m_selectedLineIdx > lineIdx - 1) {
                    m_selectedLineIdx = lineIdx - 1;
                    UpdateDisasm(m_disasm[0].addr, 2);
                }
            }
        }
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