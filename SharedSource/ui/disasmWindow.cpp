#include <format>

#include "DisasmWindow.h"
#include "Utils\StringUtils.h"

dev::DisasmWindow::DisasmWindow(
        dev::Hardware& _hardware, ImFont* fontComment, 
        const float* const _fontSize, const float* const _dpiScale)
    :
    BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSize, _dpiScale),
    m_hardware(_hardware),
    m_fontComment(fontComment)
{}

void dev::DisasmWindow::Update()
{
    BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Disasm", &open, ImGuiWindowFlags_NoCollapse);

    DrawDebugControls();
	DrawSearch();
    DrawDisassembly();

	ImGui::End();
}

void dev::DisasmWindow::DrawDebugControls()
{
    if (ImGui::Button("Step"))
    {
        m_hardware.ExecuteInstruction();
        UpdateDisasm(m_hardware.m_cpu.m_pc);
    }
    ImGui::SameLine();
    if (ImGui::Button("Step 0x100"))
    {
        for (int i = 0; i < 0x100; i++)
        {
            m_hardware.ExecuteInstruction();
        }

        UpdateDisasm(m_hardware.m_cpu.m_pc);
    }
    ImGui::SameLine();
    if (ImGui::Button("Step Frame"))
    {
        m_hardware.ExecuteFrame();
        UpdateDisasm(m_hardware.m_cpu.m_pc);
    }
}

void dev::DisasmWindow::UpdateDisasm(const GlobalAddr _globalAddr, const int _instructionsOffset)
{
    // TODO: request meaningful amount disasmm lines, not a 100!
    m_disasm = m_hardware.m_debugger.GetDisasm(_globalAddr, 80, _instructionsOffset);
}

void dev::DisasmWindow::DrawSearch()
{
    ImGui::PushItemWidth(-100);
    ImGui::InputTextWithHint("##empty 1", "0x100", m_searchText, IM_ARRAYSIZE(m_searchText));
    ImGui::SameLine(); dev::DrawHelpMarker(
        "Search by a hexadecimal address in the format of 0x100 or 100,\n"
        "or by a case-sensitive label name.");
    ImGui::PopItemWidth();
}

// Make the UI compact because there are so many fields
void PushStyleCompact()
{
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5, 3 });
}
void PopStyleCompact()
{
    ImGui::PopStyleVar(2);
}

bool dev::DisasmWindow::IsDisasmTableOutOfWindow()
{
    ImVec2 cursorPos = ImGui::GetCursorPos();
    float remainingSpace = ImGui::GetWindowSize().y - cursorPos.y - *m_fontSizeP * 1.0f;

    return remainingSpace < 0;
}

void dev::DisasmWindow::DrawDisassembly()
{
    if (m_disasm.empty()) return;

    //m_reqDisasmUpdatem = ReqDisasmUpdate::NONE;
    static int item_current_idx = 0;
    int scrollDirection = 0;

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    static ImGuiTableFlags tbl_flags = ImGuiTableFlags_NoBordersInBody;

    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

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

        /*
        ImGuiListClipper clipper;
        clipper.Begin((int)m_disasm.size());
        while (clipper.Step())
            for (int row_idx = clipper.DisplayStart; row_idx < clipper.DisplayEnd; row_idx++)
        */

        for (int line_idx = 0; line_idx < 200; line_idx++)
            {
                // TODO: fix it. replace with fixed amount of lines and without a need to check the end of the window
                if (IsDisasmTableOutOfWindow()) break;

                ImGui::TableNextRow();

                if (line_idx >= m_disasm.size()) break;

                auto& line = m_disasm[line_idx];

                bool isComment = line.type == Debugger::DisasmLine::Type::COMMENT;
                bool isCode = line.type == Debugger::DisasmLine::Type::CODE;
                int addr = line.addr;

                // the breakpoints and the execution cursor column
                char disasmLineId_s[32];
                sprintf_s(disasmLineId_s, "##disasmLineId%04d", line_idx);

                ImGui::TableNextColumn();
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DISASM_TBL_BG_COLOR_BRK);
                const bool is_selected = (item_current_idx == line_idx);
                if (ImGui::Selectable(disasmLineId_s, is_selected, ImGuiSelectableFlags_SpanAllColumns))
                {
                    item_current_idx = line_idx;
                }

                // draw breakpoints
                ImGui::SameLine();
                dev::DrawBreakpoint(true, *m_dpiScaleP);

                // draw program counter icon
                if (isCode && addr == m_hardware.m_cpu.m_pc)
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
                    if (m_fontComment) {
                        ImGui::PushFont(m_fontComment);
                    }

                    // the code column
                    ImGui::TableNextColumn();
                    ImGui::TextColored(DISASM_TBL_COLOR_COMMENT, line.str.c_str());

                    // Revert to the default font
                    if (m_fontComment) {
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
                    auto* statsColor = (line.runs == 0 && line.reads == 0 && line.writes == 0) ? &DISASM_TBL_COLOR_ZERO_STATS : &DISASM_TBL_COLOR_ADDR;
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

    // check the keys
    if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
    {
        if (m_disasm.size() >= 1) UpdateDisasm(m_disasm[0].addr, -1);
    }
    else if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
    {
        if (m_disasm.size() >= 1) UpdateDisasm(m_disasm[0].addr, 1);
    }

    if (ImGui::GetIO().MouseWheel > 0.0f)
    {
        if (m_disasm.size() >= 1) UpdateDisasm(m_disasm[0].addr, -2);
    }
    else if (ImGui::GetIO().MouseWheel < 0.0f)
    {
        if (m_disasm.size() >= 1) UpdateDisasm(m_disasm[0].addr, 2);
    }
}