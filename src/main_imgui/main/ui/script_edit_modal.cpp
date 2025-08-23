#include "ui/script_edit_modal.h"

#include <format>

#include "utils/str_utils.h"
#include "utils/imgui_utils.h"

dev::ScriptEditModal::ScriptEditModal(
	Hardware& _hardware, Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool* _visibleP, const float* const _dpiScaleP)
	:
	BaseWindow("Script Edit", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP, _dpiScaleP,
		ImGuiWindowFlags_AlwaysAutoResize,
		BaseWindow::Type::Modal),
	m_hardware(_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::SCRIPT_EDIT_WINDOW_ADD,
			std::bind(&dev::ScriptEditModal::CallbackAdd, this,
				std::placeholders::_1, std::placeholders::_2)));

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::SCRIPT_EDIT_WINDOW_EDIT,
			std::bind(&dev::ScriptEditModal::CallbackEdit, this,
				std::placeholders::_1, std::placeholders::_2)));
}

void dev::ScriptEditModal::CallbackAdd(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	auto globalAddr = Addr(std::get<GlobalAddr>(*_data));

	m_enterPressed = false;
	m_setFocus = true;
	m_script = Script();
	m_code[0] = '\0'; // erase the m_code buffer

	ImGui::OpenPopup(m_name.c_str());
}

void dev::ScriptEditModal::CallbackEdit(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	auto globalAddr = Addr(std::get<GlobalAddr>(*_data));

	m_enterPressed = false;
	m_setFocus = true;
	auto currentScript = m_debugger.GetDebugData().GetScripts().Find(globalAddr);
	if (!currentScript){
		int err = 0;
	}
	m_script = *currentScript;
	strcpy(m_code, m_script.code.c_str());

	ImGui::OpenPopup(m_name.c_str());
}

void dev::ScriptEditModal::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	static ImGuiTableFlags flags =
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_ContextMenuInBody;

	if (ImGui::BeginTable("##ContextMenuTbl", 2, flags))
	{
		ImGui::TableSetupColumn(
			"##ContextMenuTblName", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn(
			"##ContextMenuTblVal", ImGuiTableColumnFlags_WidthFixed, 800);

		// Active
		DrawProperty2EditableCheckBox(
			"Active", "##ContextActive",
			&m_script.active,
			"When true, the m_code performance is tested.");

		// comment
		bool delPressed = false;
		DrawProperty2EditableS(
			"Comment", "##ContextComment",
			&m_script.comment,
			"comment", "empty string means delete the entity",
			0, &delPressed);

		// m_code ====================
		ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
		ImGui::TableNextColumn();

		ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
		TextAligned("m_code", { 1.0f, 0.5f });
		ImGui::PopStyleColor();

		ImGui::TableNextColumn();
		ImVec2 codeSize = ImGui::GetContentRegionAvail();
		codeSize.y -= ImGui::GetStyle().FramePadding.y * 2.0f;
		codeSize.y -= ImGui::GetFrameHeightWithSpacing();

		auto entered = ImGui::InputTextMultiline(
			"##ContextCode",
			m_code,
			CODE_LEN_MAX,
			codeSize,
			ImGuiInputTextFlags_AllowTabInput |
			ImGuiInputTextFlags_AutoSelectAll
		);
		//==========================

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();

		// Warnings
		const char* warning = nullptr;

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
			m_script.code = std::string(m_code);
			m_hardware.Request(
				Hardware::Req::DEBUG_SCRIPT_ADD, m_script.ToJson());

			m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
			ImGui::CloseCurrentPopup();
		}
		if (warning) ImGui::EndDisabled();

		// Cancel button
		ImGui::SameLine(); ImGui::Text(" "); ImGui::SameLine();
		if (ImGui::Button("Cancel", buttonSize)) ImGui::CloseCurrentPopup();

		// ESC pressed
		if (ImGui::IsKeyReleased(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();

		ImGui::EndTable();
	}
}