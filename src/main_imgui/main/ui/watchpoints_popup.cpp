#include "ui/watchpoints_popup.h"

#include <format>

#include "utils/str_utils.h"
#include "utils/imgui_utils.h"

dev::WatchpointsPopup::WatchpointsPopup(
	dev::Hardware& _hardware, dev::Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool* _visibleP, const float* const _dpiScaleP)
	:
	BaseWindow("#Watchpoints Popup", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP, _dpiScaleP,
		ImGuiWindowFlags_None,
		dev::BaseWindow::Type::Popup),
	m_hardware(_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::WATCHPOINTS_POPUP_ADD,
			std::bind(&dev::WatchpointsPopup::CallbackAdd, this,
				std::placeholders::_1, std::placeholders::_2)));

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::WATCHPOINTS_POPUP_EDIT,
			std::bind(&dev::WatchpointsPopup::CallbackEdit, this,
				std::placeholders::_1, std::placeholders::_2)));
}

void dev::WatchpointsPopup::CallbackAdd(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	m_signal = dev::Signals::WATCHPOINTS_POPUP_ADD;
	m_comment = "";
	m_globalAddr = 0xFF;
	m_len = 1;

	ImGui::OpenPopup(m_name.c_str());
}

void dev::WatchpointsPopup::CallbackEdit(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	const auto id = std::get<dev::Id>(*_data);
	m_signal = dev::Signals::WATCHPOINTS_POPUP_EDIT;

	auto watchpointsJ = m_hardware.Request(
		Hardware::Req::DEBUG_WATCHPOINT_GET_ALL);

	if (watchpointsJ){
		for (const auto& watchpointJ : *watchpointsJ)
		{
			Watchpoint::Data wpData{
				watchpointJ["data0"],
				watchpointJ["data1"]
			};

			if (wpData.id != id){
				continue;
			}
			m_oldId = wpData.id;
			m_isActive = wpData.active;
			m_globalAddr = wpData.globalAddr;
			m_access = static_cast<int>(wpData.access);
			m_cond = static_cast<int>(wpData.cond);
			m_type = static_cast<int>(wpData.type);
			m_val = wpData.value;
			m_len = wpData.len;
			m_comment = watchpointJ["comment"];
		}
	}

	ImGui::OpenPopup(m_name.c_str());
}

void dev::WatchpointsPopup::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	static ImGuiTableFlags flags =
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("##WpContextMenu", 2, flags))
	{
		ImGui::TableSetupColumn(
			"##WpContextMenuName", ImGuiTableColumnFlags_WidthFixed, 150);
		ImGui::TableSetupColumn(
			"##WpContextMenuVal", ImGuiTableColumnFlags_WidthFixed, 210);

		// Status
		DrawProperty2EditableCheckBox(
			"Active", "##WpContextStatus", &m_isActive,
			"Enable or disable the watchpoint");

		// addr
		DrawProperty2EditableI("Global Address", "##WpContextAddress",
			&m_globalAddr,
			"A hexademical address in the format FF",
			ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_AutoSelectAll);

		// Access
		DrawProperty2RadioButtons("Access", &m_access,
			dev::wpAccessS, IM_ARRAYSIZE(dev::wpAccessS), 8.0f,
			"R - read, W - write, RW - read or write");

		// Condition
		DrawProperty2Combo("Condition", "##WpContextCondition",
			&m_cond, dev::ConditionsS, IM_ARRAYSIZE(dev::ConditionsS), "");

		// Value
		if (m_cond == static_cast<int>(dev::Condition::ANY)) ImGui::BeginDisabled();
		DrawProperty2EditableI("Value", "##WpContextValue", &m_val,
			"A hexademical value in the format FF",
			ImGuiInputTextFlags_CharsHexadecimal |
			ImGuiInputTextFlags_AutoSelectAll);

		if (m_cond == static_cast<int>(dev::Condition::ANY)) {
			ImGui::EndDisabled();
		}

		// Type
		DrawProperty2RadioButtons("Type", &m_type, wpTypesS, IM_ARRAYSIZE(wpTypesS), 15.0f,
			"Byte - breaks if the condition succeeds for any bytes "
			"in the defined range\n"
			"Word - breaks if the condition succeeds for a word");

		// Length
		if (m_type == static_cast<int>(Watchpoint::Type::WORD)) {
			ImGui::BeginDisabled();
			m_len = 2;
		}
		DrawProperty2EditableI("Length", "##WpContextLen", &m_len,
			"A hexademical value in the format FF",
			ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_AutoSelectAll);

			if (m_type == static_cast<int>(Watchpoint::Type::WORD)) {
			ImGui::EndDisabled();
		}

		// Comment
		DrawProperty2EditableS("Comment", "##WpContextComment", &m_comment, "");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();

		// Warnings
		std::string warningS = "";

		if (m_globalAddr >= Memory::MEMORY_GLOBAL_LEN) {
			warningS = "Too large address";
		}
		if (m_len > 0xFFFF) {
			warningS = "Too large length";
		} else{
			if (m_len <= 0) {
				warningS = "Too small length";
			}
		}
		if (m_val > 0xFFFF ||
			(m_type != static_cast<int>(Watchpoint::Type::WORD) && m_val > 0xFF))
		{
			warningS = "Too large value";
		}

		ImGui::TextColored(DASM_CLR_WARNING, warningS.c_str());

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();

		// OK button
		if (!warningS.empty()) ImGui::BeginDisabled();
		if (ImGui::Button("Ok", buttonSize))
		{
			int id = m_signal == dev::Signals::WATCHPOINTS_POPUP_ADD ?
				-1 :
				m_oldId;

			Watchpoint::Data wpData{ id, static_cast<Watchpoint::Access>(m_access),
				(GlobalAddr)m_globalAddr, static_cast<dev::Condition>(m_cond),
				(uint16_t)m_val, static_cast<Watchpoint::Type>(m_type),
				(GlobalAddr)m_len, m_isActive };

			m_hardware.Request(Hardware::Req::DEBUG_WATCHPOINT_ADD,
				{ {"data0", wpData.data0},
				{"data1", wpData.data1},
				{"comment", m_comment} });

			m_scheduler.AddSignal(
				{dev::Signals::HEX_HIGHLIGHT_ON,
				Scheduler::GlobalAddrLen{
					(GlobalAddr)m_globalAddr, (uint16_t)m_len}});
			ImGui::CloseCurrentPopup();
		}
		if (!warningS.empty()) ImGui::EndDisabled();

		// Cancel button
		ImGui::SameLine();
		ImGui::Text(" ");
		ImGui::SameLine();

		if (ImGui::Button("Cancel", buttonSize))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndTable();
	}

}