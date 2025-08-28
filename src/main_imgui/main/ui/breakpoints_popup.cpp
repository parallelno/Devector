#include "ui/breakpoints_popup.h"

#include <format>

#include "utils/str_utils.h"
#include "utils/imgui_utils.h"

dev::BreakpointsPopup::BreakpointsPopup(
	dev::Hardware& _hardware, dev::Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool* _visibleP)
	:
	BaseWindow("#Breakpoints Popup", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP,
		ImGuiWindowFlags_None,
		dev::BaseWindow::Type::Popup),
	m_hardware(_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::BREAKPOINTS_POPUP_ADD,
			std::bind(&dev::BreakpointsPopup::CallbackAdd, this,
				std::placeholders::_1, std::placeholders::_2)));

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::BREAKPOINTS_POPUP_EDIT,
			std::bind(&dev::BreakpointsPopup::CallbackEdit, this,
				std::placeholders::_1, std::placeholders::_2)));
}

void dev::BreakpointsPopup::CallbackAdd(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	m_signal = dev::Signals::BREAKPOINTS_POPUP_ADD;
	m_comment = "";
	m_addrOld = 0xFF;
	m_addr = 0xFF;
	m_val = 0;

	ImGui::OpenPopup(m_name.c_str());
}

void dev::BreakpointsPopup::CallbackEdit(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	m_addr = std::get<GlobalAddr>(*_data);
	m_signal = dev::Signals::BREAKPOINTS_POPUP_EDIT;

	auto breakpointsJ = m_hardware.Request(
		Hardware::Req::DEBUG_BREAKPOINT_GET_ALL);

	if (breakpointsJ){
		for (const auto& breakpointJ : *breakpointsJ)
		{
			Breakpoint::Data bpData{
				breakpointJ["data0"],
				breakpointJ["data1"],
				breakpointJ["data2"]
			};

			if (bpData.structured.addr != m_addr){
				continue;
			}

			m_isActive = bpData.structured.status == Breakpoint::Status::ACTIVE;
			m_addrOld = bpData.structured.addr;
			m_memPages = bpData.structured.memPages;
			m_isAutoDel = bpData.structured.autoDel;
			m_val = bpData.structured.value;
			m_selectedOp = static_cast<int>(bpData.structured.operand);
			m_selectedCond = static_cast<int>(bpData.structured.cond);
			m_comment = breakpointJ["comment"];
		}
	}

	ImGui::OpenPopup(m_name.c_str());
}

void dev::BreakpointsPopup::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	static ImGuiTableFlags flags =
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("##BPContextMenu", 2, flags))
	{
		auto scale = ImGui::GetWindowDpiScale();

		ImGui::TableSetupColumn(
			"##BPContextMenuName", ImGuiTableColumnFlags_WidthFixed, 140 * scale);
		ImGui::TableSetupColumn(
			"##BPContextMenuVal", ImGuiTableColumnFlags_WidthStretch);

		// status
		DrawProperty2EditableCheckBox(
			"Active", "##BPContextStatus",
			&m_isActive, "Enable or disable the breakpoint");

		// addr
		DrawProperty2EditableI("Address", "##WpContextAddress", &m_addr,
			"A hexademical address in the format FF",
			ImGuiInputTextFlags_CharsHexadecimal |
			ImGuiInputTextFlags_AutoSelectAll);

		// mapping
		m_memPages = DrawPropertyMemoryMapping(m_memPages);

		// auto delete
		DrawProperty2EditableCheckBox("Auto Delete",
			"##BPContextAutoDel", &m_isAutoDel,
			"Removes the breakpoint when execution halts");

		// Operand
		DrawProperty2Combo("Operand", "##BPContextOperand", &m_selectedOp,
			dev::bpOperandsS, IM_ARRAYSIZE(dev::bpOperandsS),
			"A, B, C, D, E, H, L, BC, DE, HL - CPU registers\n"\
			"Flags - CPU flags\n"\
			"SP - CPU Stack Pointer\n"\
			"CC - CPU Cicles counted from the last reset/reboot/reload");

		// Condition
		DrawProperty2Combo("Condition", "##BPContextCondition",
			&m_selectedCond, dev::ConditionsS,
			IM_ARRAYSIZE(dev::ConditionsS), "");

		// Value
		if (m_selectedCond == 0) ImGui::BeginDisabled();
		DrawProperty2EditableI("Value", "##BPContextValue", &m_val,
			"A hexademical value in the format FF",
			ImGuiInputTextFlags_CharsHexadecimal |
			ImGuiInputTextFlags_AutoSelectAll);
		if (m_selectedCond == 0) ImGui::EndDisabled();

		// Comment
		DrawProperty2EditableS("Comment", "##BPContextComment",
			&m_comment, "");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();
		// warning
		std::string warningS = "";
		warningS = m_addr >= Memory::MEMORY_MAIN_LEN ?
			"Too large address" : warningS;
		size_t maxVal =
			m_selectedOp == static_cast<int>(Breakpoint::Operand::CC) ?
			UINT64_MAX : m_selectedOp > static_cast<int>(Breakpoint::Operand::PSW) ?
			UINT16_MAX : UINT8_MAX;
		warningS = m_val < 0 || m_val > maxVal ?
			"A value is out of range" : warningS;

		ImGui::TextColored(DASM_CLR_WARNING, warningS.c_str());

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();

		if (!warningS.empty()) ImGui::BeginDisabled();
		// OK button
		if (ImGui::Button("Ok", m_buttonSize))
		{
			if (m_signal == dev::Signals::BREAKPOINTS_POPUP_EDIT &&
				m_addrOld != m_addr)
			{
				m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DEL,
					{ {"addr", m_addrOld} });
			}
			Breakpoint::Data bpData
			{
				static_cast<Addr>(m_addr),
				m_memPages,
				m_isActive ?
					Breakpoint::Status::ACTIVE :
					Breakpoint::Status::DISABLED,
				m_isAutoDel,
				static_cast<Breakpoint::Operand>(m_selectedOp),
				static_cast<dev::Condition>(m_selectedCond),
				static_cast<size_t>(m_val)
			};
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_ADD, {
				{"data0", bpData.data0 },
				{"data1", bpData.data1 },
				{"data2", bpData.data2 },
				{"comment", m_comment}
			});
			ImGui::CloseCurrentPopup();
		}
		if (!warningS.empty()) ImGui::EndDisabled();

		// Cancel button
		ImGui::SameLine();
		ImGui::Text(" ");
		ImGui::SameLine();

		if (ImGui::Button("Cancel", m_buttonSize))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndTable();
	}
}