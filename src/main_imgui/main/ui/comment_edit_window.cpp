#include "ui/comment_edit_window.h"

#include <format>

#include "utils/str_utils.h"
#include "utils/imgui_utils.h"

dev::CommentEditWindow::CommentEditWindow(
	dev::Hardware& _hardware, dev::Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool& _visible, const float* const _dpiScaleP)
	:
	BaseWindow("Comment Edit", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visible, _dpiScaleP,
		ImGuiWindowFlags_AlwaysAutoResize,
		dev::BaseWindow::Type::Popup),
	m_hardware(_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::COMMENT_EDIT_WINDOW_ADD,
			std::bind(&dev::CommentEditWindow::CallbackAdd, this,
				std::placeholders::_1, std::placeholders::_2),
			m_visible));

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::COMMENT_EDIT_WINDOW_EDIT,
			std::bind(&dev::CommentEditWindow::CallbackEdit, this,
				std::placeholders::_1, std::placeholders::_2),
			m_visible));
}

void dev::CommentEditWindow::CallbackAdd(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	auto globalAddr = Addr(std::get<GlobalAddr>(*_data));

	m_enterPressed = false;
	m_setFocus = true;
	m_addr = globalAddr;
	m_oldAddr = globalAddr;
	m_comment = "";

	m_visible = true;
	ImGui::OpenPopup(m_name.c_str());
}

void dev::CommentEditWindow::CallbackEdit(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	auto globalAddr = Addr(std::get<GlobalAddr>(*_data));

	m_enterPressed = false;
	m_setFocus = true;
	m_addr = globalAddr;
	m_oldAddr = globalAddr;
	auto commentP = m_debugger.GetDebugData().GetComment(globalAddr);
	m_comment = commentP ? *commentP : "";

	m_visible = true;
	ImGui::OpenPopup(m_name.c_str());
}

void dev::CommentEditWindow::Draw(
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
			&m_comment,
			"m_comment", "empty string means delete the m_comment",
			0, &delPressed);

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
			// empty string means a req to delete the entity
			if (m_comment.empty() || m_addr != m_oldAddr)
			{
				m_debugger.GetDebugData().DelComment(m_oldAddr);
			}
			if (!m_comment.empty()) {
				m_debugger.GetDebugData().SetComment(m_addr, m_comment);
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