#include "ui/disasm_popup.h"

#include <format>

#include "utils/str_utils.h"
#include "utils/imgui_utils.h"

dev::DisasmPopup::DisasmPopup(
	dev::Hardware& _hardware, dev::Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool* _visibleP)
	:
	BaseWindow("#Disasm Popup", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP,
		ImGuiWindowFlags_None,
		dev::BaseWindow::Type::Popup),
	m_hardware(_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::DISASM_POPUP_OPEN | dev::Signals::DISASM_POPUP_OPEN_IMM,
			std::bind(&dev::DisasmPopup::CallbackOpen, this,
				std::placeholders::_1, std::placeholders::_2)));
}

void dev::DisasmPopup::CallbackOpen(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	m_immHovered = _signals & dev::Signals::DISASM_POPUP_OPEN_IMM;
	m_globalAddr = std::get<dev::GlobalAddr>(*_data);

	m_editMemoryExists = m_debugger.GetDebugData().GetMemoryEdit(m_globalAddr);

	ImGui::OpenPopup(m_name.c_str());
}

void dev::DisasmPopup::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	if (ImGui::MenuItem("Copy"))
	{
		uint32_t cmd = m_hardware.Request(
			Hardware::Req::GET_THREE_BYTES_RAM,
			{ { "addr", m_globalAddr } })->at("data");

		uint8_t opcode = cmd & 0xFF;
		uint8_t cmd_byte1 = (cmd >> 8) & 0xFF;
		uint8_t cmd_byte2 = (cmd >> 16) & 0xFF;

		auto line = dev::GetDisasmSimpleLine(
			m_globalAddr, opcode, cmd_byte1, cmd_byte2);
		dev::CopyToClipboard(line);
	}

	ImGui::SeparatorText("");
	if (ImGui::MenuItem("Show Current Break"))
	{
		Addr regPC = m_hardware.Request(Hardware::Req::GET_REG_PC)->at("pc");
		m_scheduler.AddSignal(
			{dev::Signals::DISASM_UPDATE, (GlobalAddr)regPC});
	}

	ImGui::SeparatorText("");

	if (ImGui::MenuItem("Run To"))
	{
		Breakpoint::Data bpData{(Addr)m_globalAddr,
								Breakpoint::MAPPING_PAGES_ALL,
								Breakpoint::Status::ACTIVE,
								true};

		m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_ADD, {
			{"data0", bpData.data0 },
			{"data1", bpData.data1 },
			{"data2", bpData.data2 },
			{"comment", ""}
		});

		m_hardware.Request(Hardware::Req::RUN);
	}
	ImGui::SeparatorText("");
	if (ImGui::MenuItem("Add/Remove Beakpoint"))
	{
		Breakpoint::Status bpStatus = static_cast<Breakpoint::Status>(
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_GET_STATUS,
				{ {"addr", m_globalAddr} })->at("status"));

		if (bpStatus == Breakpoint::Status::DELETED) {
			Breakpoint::Data bpData { (Addr)m_globalAddr };
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_ADD, {
				{"data0", bpData.data0 },
				{"data1", bpData.data1 },
				{"data2", bpData.data2 },
				{"comment", ""}
				});
		}
		else {
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DEL,
				{ {"addr", m_globalAddr} });
		}
		m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
	}
	if (ImGui::MenuItem("Remove All Beakpoints")) {
		m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DEL_ALL);
		m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
	};

	ImGui::SeparatorText("");

	if (ImGui::MenuItem("Add/Edit Label"))
	{
		auto labelExists =
			m_debugger.GetDebugData().GetLabels(m_globalAddr) != std::nullopt;

		if (labelExists)
		{
			m_scheduler.AddSignal({
				dev::Signals::LABEL_EDIT_WINDOW_EDIT, m_globalAddr});
		}
		else
		{
			m_scheduler.AddSignal({
				dev::Signals::LABEL_EDIT_WINDOW_ADD, m_globalAddr});
		}

	};

	if (m_immHovered && ImGui::MenuItem("Add/Edit Const"))
	{
		auto constExists = m_debugger.GetDebugData().GetConsts(m_globalAddr);
		if (constExists)
		{
			m_scheduler.AddSignal({
				dev::Signals::CONST_EDIT_WINDOW_EDIT, m_globalAddr});
		}
		else
		{
			m_scheduler.AddSignal({
				dev::Signals::CONST_EDIT_WINDOW_ADD, m_globalAddr});
		}
	};

	if (ImGui::MenuItem("Add/Edit Comment"))
	{
		auto commentExists = m_debugger.GetDebugData().GetComment(m_globalAddr);

		if (commentExists)
		{
			m_scheduler.AddSignal({
				dev::Signals::COMMENT_EDIT_WINDOW_EDIT, m_globalAddr});
		}
		else
		{
			m_scheduler.AddSignal({
				dev::Signals::COMMENT_EDIT_WINDOW_ADD, m_globalAddr});
		}
	};

	ImGui::SeparatorText("");

	if (ImGui::MenuItem("Edit Memory"))
	{
		auto editMemoryExists = m_debugger.GetDebugData().GetMemoryEdit(m_globalAddr);

		if (editMemoryExists)
		{
			m_scheduler.AddSignal({
				dev::Signals::MEMORY_EDIT_WINDOW_EDIT, m_globalAddr});
		}
		else
		{
			m_scheduler.AddSignal({
				dev::Signals::MEMORY_EDIT_WINDOW_ADD, m_globalAddr});
		}
	}

	if (m_editMemoryExists && ImGui::MenuItem(
		"Cancel Edit Memory at This Addr"))
	{
		m_hardware.Request(
			Hardware::Req::DEBUG_MEMORY_EDIT_DEL,
			{ {"addr", m_globalAddr} });

		m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
	}

	if (ImGui::MenuItem("Code Perf"))
	{
		auto codePerfExists = m_debugger.GetDebugData().GetCodePerf(m_globalAddr);

		if (codePerfExists)
		{
			m_scheduler.AddSignal({
				dev::Signals::CODE_PERF_EDIT_WINDOW_EDIT, m_globalAddr});
		}
		else
		{
			m_scheduler.AddSignal({
				dev::Signals::CODE_PERF_EDIT_WINDOW_ADD, m_globalAddr});
		}
	}
}