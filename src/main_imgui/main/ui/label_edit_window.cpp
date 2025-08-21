#include "ui/label_edit_window.h"

#include <format>

#include "utils/str_utils.h"
#include "utils/imgui_utils.h"

dev::LabelEditWindow::LabelEditWindow(
	Hardware& _hardware, Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool& _visible, const float* const _dpiScaleP)
	:
	BaseWindow("Label Edit", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visible, _dpiScaleP,
		ImGuiWindowFlags_AlwaysAutoResize,
		BaseWindow::Type::Popup),
	m_hardware(_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::LABEL_EDIT_WINDOW_ADD,
			std::bind(&dev::LabelEditWindow::CallbackAdd, this,
				std::placeholders::_1, std::placeholders::_2),
			m_visible));

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::LABEL_EDIT_WINDOW_EDIT,
			std::bind(&dev::LabelEditWindow::CallbackEdit, this,
				std::placeholders::_1, std::placeholders::_2),
			m_visible));
}

void dev::LabelEditWindow::CallbackAdd(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	auto globalAddr = Addr(std::get<GlobalAddr>(*_data));

	m_enterPressed = false;
	m_setFocus = true;
	m_addr = globalAddr;
	m_oldAddr = globalAddr;
	m_labels.clear();
	m_labels.push_back("");
	m_selectedItemIdx = 0;
	m_editLabel = false;

	m_visible = true;
	ImGui::OpenPopup(m_name.c_str());
}

void dev::LabelEditWindow::CallbackEdit(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	auto globalAddr = Addr(std::get<GlobalAddr>(*_data));

	m_enterPressed = false;
	m_setFocus = true;
	m_addr = globalAddr;
	m_oldAddr = globalAddr;
	auto labelsP = m_debugger.GetDebugData().GetLabels(globalAddr);
	if (labelsP) {
		m_labels = std::move(*labelsP);
	}
	m_selectedItemIdx = 0;
	m_editLabel = true;

	m_visible = true;
	ImGui::OpenPopup(m_name.c_str());
}

void dev::LabelEditWindow::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	static ImGuiTableFlags flags =
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("##ContextMenuTbl", 2, flags))
	{
		ImGui::TableSetupColumn(
			"##ContextMenuTblName", ImGuiTableColumnFlags_WidthFixed, 150);
		ImGui::TableSetupColumn(
			"##ContextMenuTblVal", ImGuiTableColumnFlags_WidthFixed, 200);

		// Comment
		bool delPressed = false;
		if (DrawProperty2EditableS(
				"Name", "##ContextComment",
				&(m_labels[m_selectedItemIdx]),
				"name", "empty string means delete the label",
				0, &delPressed))
		{
			// replace spaces with '_' in the name
			std::replace(
				m_labels[m_selectedItemIdx].begin(),
				m_labels[m_selectedItemIdx].end(), ' ', '_');
		}
		if (delPressed)
		{
			dev::DeleteByIndex(m_labels, m_selectedItemIdx);
		}

		// Global Addr
		if (m_setFocus) { ImGui::SetKeyboardFocusHere(); m_setFocus = false; }

		DrawProperty2EditableI("Global Address", "##EMContextAddress", &m_addr,
			"A hexademical address in the format FF",
			ImGuiInputTextFlags_CharsHexadecimal |
			ImGuiInputTextFlags_AutoSelectAll);

		m_enterPressed |= ImGui::IsKeyPressed(ImGuiKey_Enter);


		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::SeparatorText("");
		ImGui::TableNextColumn();
		ImGui::SeparatorText("");

		// list all m_labels
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();

		if (m_labels.size() > 1)
		{
			if (ImGui::BeginListBox("##LListBox"))
			{
				for (int labelIdx = 0; labelIdx < m_labels.size(); labelIdx++)
				{
					auto& label = m_labels[labelIdx];

					if (label.empty()) continue;

					const bool is_selected = (m_selectedItemIdx == labelIdx);
					if (ImGui::Selectable(
							std::format("{}##{}", label, labelIdx).c_str(),
							is_selected))
					{
						m_selectedItemIdx = labelIdx;
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
				"This list contains all m_labels with the same value.\n"
				"Specify which label fits this context best.");
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

		if (ImGui::Button("Ok", buttonSize) || m_enterPressed)
		{
			// remove empty m_labels
			m_labels.erase(std::remove_if(m_labels.begin(), m_labels.end(),
				[](const std::string& label) { return label.empty(); }), m_labels.end());

			// empty list of strings means a req to delete the entity
			if (m_editLabel && (m_labels.empty() || m_addr != m_oldAddr))
			{
				m_debugger.GetDebugData().DelLabels(m_oldAddr);
			}

			// merge edited m_labels with existing m_labels
			if (!m_editLabel || m_addr != m_oldAddr)
			{
				auto labelsP = m_debugger.GetDebugData().GetLabels(m_addr);
				if (labelsP) {
					for (auto& label : *labelsP) {
						if (std::find(m_labels.begin(), m_labels.end(), label) == m_labels.end()) {
							m_labels.push_back(label);
						}
					}
				}
			}

			// store m_labels
			if (!m_labels.empty()) {
				m_debugger.GetDebugData().SetLabels(m_addr, m_labels);
			}

			m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
			ImGui::CloseCurrentPopup();
		}
		if (warning) ImGui::EndDisabled();

		// Cancel button
		ImGui::SameLine();
		ImGui::Text(" ");
		ImGui::SameLine();

		if (ImGui::Button("Cancel", buttonSize) |
			ImGui::IsKeyReleased(ImGuiKey_Escape))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndTable();
	}
}