#include "ui/trace_log_popup.h"

#include <format>

#include "utils/str_utils.h"
#include "utils/imgui_utils.h"

dev::TraceLogPopup::TraceLogPopup(
	dev::Hardware& _hardware, dev::Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool* _visibleP, const float* const _dpiScaleP)
	:
	BaseWindow("#Tracelog Popup", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP, _dpiScaleP,
		ImGuiWindowFlags_None,
		dev::BaseWindow::Type::Popup),
	m_hardware(_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::TRACE_LOG_POPUP_OPEN,
			std::bind(&dev::TraceLogPopup::CallbackOpen, this,
				std::placeholders::_1, std::placeholders::_2)));
}

void dev::TraceLogPopup::CallbackOpen(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	m_addr = Addr(std::get<GlobalAddr>(*_data));
	m_str = std::format("0x{:04x}", m_addr);

	ImGui::OpenPopup(m_name.c_str());
}


void dev::TraceLogPopup::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	if (ImGui::MenuItem("Copy")) {
		dev::CopyToClipboard(m_str);
	}

	ImGui::SeparatorText("");
	if (ImGui::MenuItem("Add/Remove Beakpoint"))
	{
		Breakpoint::Status bpStatus =
			static_cast<Breakpoint::Status>(
				m_hardware.Request(
					Hardware::Req::DEBUG_BREAKPOINT_GET_STATUS,
					{ {"addr", m_addr} })->at("status"));

		if (bpStatus == Breakpoint::Status::DELETED) {
			Breakpoint::Data bpData { m_addr };
			m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_ADD, {
				{"data0", bpData.data0 },
				{"data1", bpData.data1 },
				{"data2", bpData.data2 },
				{"comment", ""}
				});
		}
		else {
			m_hardware.Request(
				Hardware::Req::DEBUG_BREAKPOINT_DEL,
				{ {"addr", m_addr} });
		}
	}
}