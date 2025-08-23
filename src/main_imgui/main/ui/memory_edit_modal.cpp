#include "ui/memory_edit_modal.h"

#include <format>

#include "utils/str_utils.h"
#include "utils/imgui_utils.h"

dev::MemoryEditWindow::MemoryEditWindow(
	Hardware& _hardware, Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool* _visibleP, const float* const _dpiScaleP)
	:
	BaseWindow("Memory Edit", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP, _dpiScaleP,
		ImGuiWindowFlags_AlwaysAutoResize,
		BaseWindow::Type::Modal),
	m_hardware(_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::MEMORY_EDIT_WINDOW_ADD,
			std::bind(&dev::MemoryEditWindow::CallbackAdd, this,
				std::placeholders::_1, std::placeholders::_2)));

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::MEMORY_EDIT_WINDOW_EDIT,
			std::bind(&dev::MemoryEditWindow::CallbackEdit, this,
				std::placeholders::_1, std::placeholders::_2)));
}

void dev::MemoryEditWindow::CallbackAdd(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	auto m_setFocus = Addr(std::get<GlobalAddr>(*_data));

	m_enterPressed = false;
	m_setFocus = true;
	m_globalAddr = m_setFocus;
	m_edit.Erase();
	m_value = 0;

	ImGui::OpenPopup(m_name.c_str());
}

void dev::MemoryEditWindow::CallbackEdit(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	auto m_setFocus = Addr(std::get<GlobalAddr>(*_data));

	m_enterPressed = false;
	m_setFocus = true;
	m_globalAddr = m_setFocus;
	auto currentEdit = m_debugger.GetDebugData().GetMemoryEdit(m_setFocus);
	m_edit = *currentEdit;
	m_value = m_edit.value;

	ImGui::OpenPopup(m_name.c_str());
}

void dev::MemoryEditWindow::Draw(
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
		DrawProperty2EditableS(
			"Comment", "##ContextComment",
			&m_edit.comment,
			"comment", "empty string means delete the entity",
			0, &delPressed);

		// Global Addr
		if (m_setFocus) {
			ImGui::SetKeyboardFocusHere(); m_setFocus = false;
		}
		DrawProperty2EditableI("Global Address", "##EMContextAddress",
			&m_globalAddr,
			"A hexademical address in the format FF",
			ImGuiInputTextFlags_CharsHexadecimal |
			ImGuiInputTextFlags_AutoSelectAll);

		m_enterPressed |= ImGui::IsKeyPressed(ImGuiKey_Enter);

		// Value
		DrawProperty2EditableI("Value", "##ContextValue", &m_value,
			"A byte m_value in a hexademical format FF",
			ImGuiInputTextFlags_CharsHexadecimal |
			ImGuiInputTextFlags_AutoSelectAll);

		m_enterPressed |= ImGui::IsKeyPressed(ImGuiKey_Enter);

		// readonly
		DrawProperty2EditableCheckBox("Read-Only", "##ContextReadOnly",
			&m_edit.readonly,
			"When true, the hardware cannot override this m_value.\n"
			"Otherwise, the hardware can change the stored m_value.");

		// Active
		DrawProperty2EditableCheckBox("Active", "##ContextActive",
			&m_edit.active, "When true, the memory is edited.");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();

		// Warnings
		const char* warning = nullptr;

		if (m_globalAddr >= Memory::MEMORY_GLOBAL_LEN) {
			warning = "Too large address";
		}
		else if (m_globalAddr < 0) {
			warning = "Too low address";
		}
		else if (m_value > 0xFF) {
			warning = "Too large m_value";
		}
		else if (m_value < 0) {
			warning = "Too low m_value";
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
			auto oldAddr = m_edit.globalAddr;
			m_edit.globalAddr = m_globalAddr;
			m_edit.value = m_value;

			if (m_globalAddr != oldAddr)
			{
				m_hardware.Request(
					Hardware::Req::DEBUG_MEMORY_EDIT_DEL,
					{ {"addr", oldAddr} });
			}
			m_hardware.Request(
				Hardware::Req::DEBUG_MEMORY_EDIT_ADD, m_edit.ToJson());

			// inject memory edits
			if (m_edit.active) {
				m_hardware.Request(
					Hardware::Req::SET_BYTE_GLOBAL,
					{{"addr", m_edit.globalAddr}, {"data", m_edit.value}});
			}

			m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
			ImGui::CloseCurrentPopup();
		}
		if (warning) {
			ImGui::EndDisabled();
		}

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