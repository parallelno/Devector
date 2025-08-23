#include "ui/code_perf_edit_modal.h"

#include <format>

#include "utils/str_utils.h"
#include "utils/imgui_utils.h"

dev::CodePerfEditModal::CodePerfEditModal(
	Hardware& m_hardware, Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool* _visibleP, const float* const _dpiScaleP)
	:
	BaseWindow("Code Perf Edit", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP, _dpiScaleP,
		ImGuiWindowFlags_AlwaysAutoResize,
		BaseWindow::Type::Modal),
	m_hardware(m_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::CODE_PERF_EDIT_WINDOW_ADD,
			std::bind(&dev::CodePerfEditModal::CallbackAdd, this,
				std::placeholders::_1, std::placeholders::_2)));

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::CODE_PERF_EDIT_WINDOW_EDIT,
			std::bind(&dev::CodePerfEditModal::CallbackEdit, this,
				std::placeholders::_1, std::placeholders::_2)));
}

void dev::CodePerfEditModal::CallbackAdd(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	auto globalAddr = Addr(std::get<GlobalAddr>(*_data));

	m_enterPressed = false;
	m_setFocus = true;
	m_addrStart = globalAddr;
	m_addrEnd = globalAddr + 0x100;
	m_codePerf.Erase();

	ImGui::OpenPopup(m_name.c_str());
}

void dev::CodePerfEditModal::CallbackEdit(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	auto globalAddr = Addr(std::get<GlobalAddr>(*_data));

	m_enterPressed = false;
	m_setFocus = true;
	m_addrStart = globalAddr;
	auto currentCodePerf = m_debugger.GetDebugData().GetCodePerf(globalAddr);
	m_codePerf = *currentCodePerf;
	m_addrEnd = m_codePerf.addrEnd;

	ImGui::OpenPopup(m_name.c_str());
}

void dev::CodePerfEditModal::Draw(
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

		// Label
		bool delPressed = false;
		DrawProperty2EditableS(
			"Label", "##ContextComment",
			&m_codePerf.label,
			"label", "empty string means delete the entity",
			0, &delPressed);

		// Global Addr Start
		if (m_setFocus) {
			ImGui::SetKeyboardFocusHere(); m_setFocus = false;
		}
		DrawProperty2EditableI(
			"Global Address Start", "##EMContextAddressStart",
			&m_addrStart,
			"A hexademical address in the format FF",
			ImGuiInputTextFlags_CharsHexadecimal |
			ImGuiInputTextFlags_AutoSelectAll);

		m_enterPressed |= ImGui::IsKeyPressed(ImGuiKey_Enter);

		// Global Addr Start
		if (m_setFocus) {
			ImGui::SetKeyboardFocusHere(); m_setFocus = false;
		}
		DrawProperty2EditableI("Global Address End", "##EMContextAddressEnd",
			&m_addrEnd,
			"A hexademical address in the format FF",
			ImGuiInputTextFlags_CharsHexadecimal |
			ImGuiInputTextFlags_AutoSelectAll);

		m_enterPressed |= ImGui::IsKeyPressed(ImGuiKey_Enter);

		// Active
		DrawProperty2EditableCheckBox("Active", "##ContextActive",
			&m_codePerf.active, "When true, the code performance is tested.");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TableNextColumn();

		// Warnings
		const char* warning = nullptr;

		if (m_addrStart >= Memory::MEMORY_GLOBAL_LEN ||
			m_addrEnd >= Memory::MEMORY_GLOBAL_LEN)
		{
			warning = "Too large address";
		}
		else if (m_addrStart < 0 || m_addrEnd < 0) {
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
			// global addr start
			auto oldAddrStart = m_codePerf.addrStart;
			m_codePerf.addrStart = m_addrStart;
			auto oldAddrEnd = m_codePerf.addrEnd;
			m_codePerf.addrEnd = m_addrEnd;
			if (m_addrStart != oldAddrStart ||
				m_addrEnd != oldAddrEnd)
			{
				m_hardware.Request(
					Hardware::Req::DEBUG_CODE_PERF_DEL,
					{ {"addr", oldAddrStart} });
			}
			m_hardware.Request(
				Hardware::Req::DEBUG_CODE_PERF_ADD,
				m_codePerf.ToJson());

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