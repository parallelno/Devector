#include "ui/const_edit_modal.h"

#include <format>

#include "utils/str_utils.h"
#include "utils/imgui_utils.h"

dev::ConstEditModal::ConstEditModal(
	dev::Hardware& _hardware, dev::Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool* _visibleP)
	:
	BaseWindow("Const Edit", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP,
		ImGuiWindowFlags_AlwaysAutoResize,
		BaseWindow::Type::Modal),
	m_hardware(_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::CONST_EDIT_WINDOW_ADD,
			std::bind(&dev::ConstEditModal::CallbackAdd, this,
				std::placeholders::_1, std::placeholders::_2)));

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::CONST_EDIT_WINDOW_EDIT,
			std::bind(&dev::ConstEditModal::CallbackEdit, this,
				std::placeholders::_1, std::placeholders::_2)));
}

void dev::ConstEditModal::CallbackAdd(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	auto globalAddr = std::get<GlobalAddr>(*_data);

	m_enterPressed = false;
	m_setFocus = true;
	m_addr = globalAddr;
	m_oldAddr = globalAddr;
	m_consts.clear();
	m_consts.push_back("");
	m_selectedItemIdx = 0;
	m_editConst = false;

	ImGui::OpenPopup(m_name.c_str());
}

void dev::ConstEditModal::CallbackEdit(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	auto globalAddr = Addr(std::get<GlobalAddr>(*_data));

	m_enterPressed = false;
	m_setFocus = true;
	m_addr = globalAddr;
	m_oldAddr = globalAddr;
	auto constsP = m_debugger.GetDebugData().GetConsts(globalAddr);
	if (constsP) {
		m_consts = std::move(*constsP);
	}
	m_selectedItemIdx = 0;
	m_editConst = true;

	ImGui::OpenPopup(m_name.c_str());
}

void dev::ConstEditModal::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	static ImGuiTableFlags flags =
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("##ContextMenuTbl", 2, flags))
	{
		auto scale = ImGui::GetWindowDpiScale();

		ImGui::TableSetupColumn(
			"##ContextMenuTblName", ImGuiTableColumnFlags_WidthFixed, 150 * scale);
		ImGui::TableSetupColumn(
			"##ContextMenuTblVal", ImGuiTableColumnFlags_WidthStretch);

		// Comment
		bool delPressed = false;
		if (dev::DrawProperty2EditableS(
			"Name", "##ContextComment",
			&(m_consts[m_selectedItemIdx]),
			"name", "empty string means delete the const",
			ImGuiInputTextFlags_CharsUppercase, &delPressed))
		{
			// replace spaces with '_' in the name
			std::replace(
				m_consts[m_selectedItemIdx].begin(),
				m_consts[m_selectedItemIdx].end(), ' ', '_');
		}
		if (delPressed)
		{
			dev::DeleteByIndex(m_consts, m_selectedItemIdx);
		}

		// Global Addr
		if (m_setFocus) {
			ImGui::SetKeyboardFocusHere(); m_setFocus = false;
		}
		DrawProperty2EditableI(
			"Global Address", "##EMContextAddress", &m_addr,
			"A hexademical address in the format FF",
			ImGuiInputTextFlags_CharsHexadecimal |
			ImGuiInputTextFlags_AutoSelectAll);
		m_enterPressed |= ImGui::IsKeyPressed(ImGuiKey_Enter);


		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::SeparatorText("");
		ImGui::TableNextColumn();
		ImGui::SeparatorText("");

		// list all m_consts
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();

		if (m_consts.size() > 1)
		{
			if (ImGui::BeginListBox("##LListBox"))
			{
				for (int constIdx = 0;
					constIdx < m_consts.size(); constIdx++)
				{
					auto& const_ = m_consts[constIdx];

					if (const_.empty()) continue;

					const bool is_selected = (m_selectedItemIdx == constIdx);
					if (ImGui::Selectable(
							std::format("{}##{}", const_, constIdx).c_str(),
							is_selected))
					{
						m_selectedItemIdx = constIdx;
					}

					// Set the initial focus when opening the combo
					// (scrolling + keyboard navigation focus)
					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}

				}
				ImGui::EndListBox();
			}
			ImGui::SameLine();
			ImGui::Dummy(UI_LITTLE_SPACE);
			ImGui::SameLine();

			dev::DrawHelpMarker(
				"This list contains all m_consts with the same value.\n"
				"Specify which const_ fits this context best.");
			ImGui::SeparatorText("");
		}


		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();

		// Warnings
		const char* warning = nullptr;

		if (m_addr >= Memory::MEMORY_MAIN_LEN) {
			warning = "Too large address";
		}
		else if (m_addr < 0) {
			warning = "Too low address";
		}

		if (warning) {
			ImGui::TextColored(DASM_CLR_WARNING, warning);
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();

		// OK button
		if (warning) ImGui::BeginDisabled();
		if (ImGui::Button("Ok", m_buttonSize) || m_enterPressed)
		{
			// remove empty m_consts
			m_consts.erase(
				std::remove_if(m_consts.begin(), m_consts.end(),
				[](const std::string& const_) { return const_.empty(); }),
				m_consts.end());

			// empty list of strings means a req to delete the entity
			if (m_editConst && (m_consts.empty() || m_addr != m_oldAddr))
			{
				m_debugger.GetDebugData().DelConsts(m_oldAddr);
			}

			// merge edited m_consts with existing m_consts
			if (!m_editConst || m_addr != m_oldAddr)
			{
				auto constsP = m_debugger.GetDebugData().GetConsts(m_addr);
				if (constsP) {
					for (auto& const_ : *constsP)
					{
						if (std::find(m_consts.begin(),
							m_consts.end(), const_) == m_consts.end())
						{
							m_consts.push_back(const_);
						}
					}
				}
			}

			// store m_consts
			if (!m_consts.empty()) {
				m_debugger.GetDebugData().SetConsts(m_addr, m_consts);
			}

			m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
			ImGui::CloseCurrentPopup();
		}
		if (warning) ImGui::EndDisabled();

		// Cancel button
		ImGui::SameLine();
		ImGui::Text(" ");
		ImGui::SameLine();

		if (ImGui::Button("Cancel", m_buttonSize) |
			ImGui::IsKeyReleased(ImGuiKey_Escape))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndTable();
	}
}