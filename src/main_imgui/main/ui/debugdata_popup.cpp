#include "ui/debugdata_popup.h"

#include <format>

#include "utils/str_utils.h"
#include "utils/imgui_utils.h"

dev::DebugDataPopup::DebugDataPopup(
	dev::Hardware& _hardware, dev::Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool* _visibleP)
	:
	BaseWindow("#Debug Data Popup", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP,
		ImGuiWindowFlags_None,
		dev::BaseWindow::Type::Popup),
	m_hardware(_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::DEBUG_DATA_POPUP_OPEN |
			dev::Signals::DEBUG_DATA_POPUP_OPEN_HOVER,
			std::bind(&dev::DebugDataPopup::CallbackOpen, this,
				std::placeholders::_1, std::placeholders::_2)));
}

void dev::DebugDataPopup::CallbackOpen(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	m_elementHovered = _signals & dev::Signals::DEBUG_DATA_POPUP_OPEN_HOVER;

	auto eNameGlobalAddrEtype = std::get<Scheduler::StrGlobalAddrId>(*_data);
	m_elementName = eNameGlobalAddrEtype.str;
	m_globalAddr = eNameGlobalAddrEtype.globalAddr;
	m_elementType = static_cast<ElementType>(eNameGlobalAddrEtype.id);

	ImGui::OpenPopup(m_name.c_str());
}

void dev::DebugDataPopup::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	if (m_elementHovered)
	{
		if (ImGui::MenuItem("Copy Name")) {
			dev::CopyToClipboard(m_elementName);
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::MenuItem("Copy Addr")) {
			dev::CopyToClipboard(std::format("0x{:X}", (m_globalAddr)));
			ImGui::CloseCurrentPopup();
		}

		ImGui::SeparatorText("");

		if (ImGui::MenuItem("Locate in the Disasm Window"))
		{
			m_scheduler.AddSignal(
				{dev::Signals::DISASM_UPDATE, (GlobalAddr)m_globalAddr});
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::MenuItem("Locate in the Hex WIndow"))
		{
			m_scheduler.AddSignal(
				{dev::Signals::HEX_HIGHLIGHT_ON,
				dev::Scheduler::GlobalAddrLen(m_globalAddr, 1)});
			ImGui::CloseCurrentPopup();
		}

		ImGui::SeparatorText("");
	}

	if (ImGui::MenuItem("Add"))
	{
		ImGui::CloseCurrentPopup();

		dev::Signals signals = dev::Signals::NONE;

		switch (m_elementType)
		{
		case ElementType::LABEL:
			signals = dev::Signals::LABEL_EDIT_WINDOW_ADD;
			break;
		case ElementType::CONST:
			signals = dev::Signals::CONST_EDIT_WINDOW_ADD;
			break;
		case ElementType::COMMENT:
			signals = dev::Signals::COMMENT_EDIT_WINDOW_ADD;
			break;
		case ElementType::MEMORY_EDIT:
			signals = dev::Signals::MEMORY_EDIT_WINDOW_ADD;
			break;
		case ElementType::CODE_PERFS:
			signals = dev::Signals::CODE_PERF_EDIT_WINDOW_ADD;
			break;
		case ElementType::SCRIPTS:
			signals = dev::Signals::SCRIPT_EDIT_WINDOW_ADD;
			break;
		}
		m_scheduler.AddSignal({signals, (GlobalAddr)0});
	}

	if (m_elementHovered)
	{
		if (ImGui::MenuItem("Edit"))
		{
			ImGui::CloseCurrentPopup();

			dev::Signals signals = dev::Signals::NONE;

			switch (m_elementType)
			{
			case ElementType::LABEL:
				signals = dev::Signals::LABEL_EDIT_WINDOW_EDIT;
				break;
			case ElementType::CONST:
				signals = dev::Signals::CONST_EDIT_WINDOW_EDIT;
				break;
			case ElementType::COMMENT:
				signals = dev::Signals::COMMENT_EDIT_WINDOW_EDIT;
				break;
			case ElementType::MEMORY_EDIT:
				signals = dev::Signals::MEMORY_EDIT_WINDOW_EDIT;
				break;
			case ElementType::CODE_PERFS:
				signals = dev::Signals::CODE_PERF_EDIT_WINDOW_EDIT;
				break;
			case ElementType::SCRIPTS:
				signals = dev::Signals::SCRIPT_EDIT_WINDOW_EDIT;
				break;
			}
			m_scheduler.AddSignal({signals, (GlobalAddr)m_globalAddr});
		}

		if (ImGui::MenuItem("Delete"))
		{
			switch (m_elementType)
			{
			case ElementType::LABEL:
				m_debugger.GetDebugData().DelLabel(
					m_globalAddr, m_elementName);
				break;

			case ElementType::CONST:
				m_debugger.GetDebugData().DelConst(
					m_globalAddr, m_elementName);
				break;

			case ElementType::COMMENT:
				m_debugger.GetDebugData().DelComment(m_globalAddr);
				break;

			case ElementType::MEMORY_EDIT:
				m_debugger.GetDebugData().DelMemoryEdit(m_globalAddr);
				break;

			case ElementType::CODE_PERFS:
				m_debugger.GetDebugData().DelCodePerf(m_globalAddr);
				break;
			case ElementType::SCRIPTS:
				m_debugger.GetDebugData().GetScripts().Del(m_globalAddr);
				break;

			default:
				break;
			}
			m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
			ImGui::CloseCurrentPopup();
		}

		ImGui::SeparatorText("");
	}

	if (ImGui::MenuItem("Delete All"))
	{
		switch (m_elementType)
		{
		case ElementType::LABEL:
			m_debugger.GetDebugData().DelAllLabels();
			break;

		case ElementType::CONST:
			m_debugger.GetDebugData().DelAllConsts();
			break;

		case ElementType::COMMENT:
			m_debugger.GetDebugData().DelAllComments();
			break;

		case ElementType::MEMORY_EDIT:
			m_hardware.Request(Hardware::Req::DEBUG_MEMORY_EDIT_DEL_ALL);
			break;

		case ElementType::CODE_PERFS:
			m_hardware.Request(Hardware::Req::DEBUG_CODE_PERF_DEL_ALL);
			break;
		case ElementType::SCRIPTS:
			m_hardware.Request(Hardware::Req::DEBUG_SCRIPT_DEL_ALL);
			break;

		default:
			break;
		}

		m_scheduler.AddSignal({dev::Signals::DISASM_UPDATE});
		ImGui::CloseCurrentPopup();
	}
}