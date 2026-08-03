#include "ui/script_edit_modal.h"

#include <algorithm>
#include <cstdio>
#include <format>

#include "utils/str_utils.h"
#include "utils/imgui_utils.h"
#include "imgui_internal.h"

dev::ScriptEditModal::ScriptEditModal(
	Hardware& _hardware, Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool* _visibleP)
	:
	BaseWindow("Script Edit", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP,
		ImGuiWindowFlags_NoCollapse,
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
	auto globalAddr = std::get<GlobalAddr>(*_data);

	m_enterPressed = false;
	m_setFocus = true;
	m_codeScrollY = 0.0f;
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
	m_codeScrollY = 0.0f;
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
		auto scale = ImGui::GetWindowDpiScale();

		ImGui::TableSetupColumn(
			"##ContextMenuTblName", ImGuiTableColumnFlags_WidthFixed, 80 * scale);
		ImGui::TableSetupColumn(
			"##ContextMenuTblVal", ImGuiTableColumnFlags_WidthStretch);

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
		TextAligned("Code", { 1.0f, 0.5f });
		ImGui::PopStyleColor();

		ImGui::TableNextColumn();
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImVec2 available = ImGui::GetContentRegionAvail();
		const float codeHeight = std::max(
			120.0f * scale,
			available.y - m_buttonSize.y - style.ItemSpacing.y * 5.0f);
		const int lineCount = 1 + static_cast<int>(
			std::count(m_code, m_code + strlen(m_code), '\n'));
		const float gutterWidth = ImGui::CalcTextSize(
			std::to_string(lineCount).c_str()).x + style.FramePadding.x * 3.0f;
		const ImVec2 gutterPos = ImGui::GetCursorScreenPos();

		ImGui::Dummy({ gutterWidth, codeHeight });
		ImGui::SameLine(0.0f, 0.0f);
		const ImGuiID codeId = ImGui::GetID("##ContextCode");
		ImGui::InputTextMultiline(
			"##ContextCode",
			m_code,
			CODE_LEN_MAX,
			{ std::max(120.0f * scale, available.x - gutterWidth), codeHeight },
			ImGuiInputTextFlags_AllowTabInput |
			ImGuiInputTextFlags_AutoSelectAll
		);

		if (const ImGuiInputTextState* state = ImGui::GetInputTextState(codeId))
			m_codeScrollY = state->Scroll.y;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 gutterMax = { gutterPos.x + gutterWidth, gutterPos.y + codeHeight };
		drawList->AddRectFilled(gutterPos, gutterMax,
			ImGui::GetColorU32(ImGuiCol_FrameBg));
		drawList->PushClipRect(gutterPos, gutterMax, true);
		const float lineHeight = ImGui::GetTextLineHeight();
		const float firstLineY = gutterPos.y + style.FramePadding.y - m_codeScrollY;
		for (int line = 0; line < lineCount; ++line)
		{
			const float lineY = firstLineY + line * lineHeight;
			if (lineY + lineHeight < gutterPos.y || lineY > gutterMax.y)
				continue;

			char lineNumber[16];
			std::snprintf(lineNumber, sizeof(lineNumber), "%d", line + 1);
			const float textWidth = ImGui::CalcTextSize(lineNumber).x;
			drawList->AddText(
				{ gutterMax.x - style.FramePadding.x - textWidth, lineY },
				ImGui::GetColorU32(ImGuiCol_TextDisabled), lineNumber);
		}
		drawList->PopClipRect();
		drawList->AddLine(
			{ gutterMax.x - 1.0f, gutterPos.y },
			{ gutterMax.x - 1.0f, gutterMax.y },
			ImGui::GetColorU32(ImGuiCol_Border));
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
		if (ImGui::Button("Ok", m_buttonSize) || m_enterPressed)
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
		if (ImGui::Button("Cancel", m_buttonSize)) ImGui::CloseCurrentPopup();

		// ESC pressed
		if (ImGui::IsKeyReleased(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();

		ImGui::EndTable();
	}
}