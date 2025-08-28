#include "ui/recorder_window.h"

#include <format>
#include "utils/str_utils.h"

dev::RecorderWindow::RecorderWindow(Hardware& _hardware, Debugger& _debugger,
	dev::Scheduler& _scheduler,
	bool* _visibleP)
	:
	BaseWindow("Recorder", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP),
	m_hardware(_hardware), m_debugger(_debugger)
{

	_scheduler.AddCallback(
		dev::Scheduler::Callback(
			dev::Signals::HW_RUNNING | dev::Signals::BREAK,
			std::bind(&dev::RecorderWindow::CallbackUpdateData,
				this, std::placeholders::_1, std::placeholders::_2),
			m_visibleP, 300ms));
}

void dev::RecorderWindow::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	bool isRunning = dev::Signals::HW_RUNNING & _signals;

	ImGui::Text("State current / recorded: %d / %d", m_stateCurrent, m_stateRecorded);

	ImGui::Separator();

	if (ImGui::Button(isRunning ? "Break" : " Run "))
	{
		m_hardware.Request(isRunning ? Hardware::Req::STOP : Hardware::Req::RUN);
	}

	if (isRunning) ImGui::BeginDisabled();

	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		m_hardware.Request(Hardware::Req::DEBUG_RECORDER_RESET);
		m_stateRecorded = m_hardware.Request(Hardware::Req::DEBUG_RECORDER_GET_STATE_RECORDED)->at("states");
		m_stateCurrent = m_hardware.Request(Hardware::Req::DEBUG_RECORDER_GET_STATE_CURRENT)->at("states");
	}
	if (isRunning) ImGui::EndDisabled();

	ImGui::SameLine();
	dev::DrawHelpMarker("Left Ctrl + R - reverse playback. Works runtime and while break\n"
						"Left Ctrl + F - forward playback. Works while break only");

	ImGui::Dummy({ 1, 5 });

	// Frame slider
	dev::PushStyleCompact(0.5f);
	int stateCurrentOld = m_stateCurrent;
	ImGui::SetNextItemWidth(-70.0f);
	if (ImGui::SliderInt("##recTimeline", &m_stateCurrent, 1, m_stateRecorded, "%d", ImGuiSliderFlags_AlwaysClamp))
	{
		int diff = m_stateCurrent - stateCurrentOld;
		if (diff < 0) {
			m_hardware.Request(Hardware::Req::DEBUG_RECORDER_PLAY_REVERSE, { {"frames", abs(diff)} });
		}
		else {
			m_hardware.Request(Hardware::Req::DEBUG_RECORDER_PLAY_FORWARD, { {"frames", abs(diff)} });
		}

		m_stateCurrent = m_hardware.Request(Hardware::Req::DEBUG_RECORDER_GET_STATE_CURRENT)->at("states");
	}

	ImGui::SameLine();
	if (ImGui::Button(" - ") ||
		(ImGui::IsKeyPressed(ImGuiKey_R) && ImGui::IsKeyPressed(ImGuiKey_LeftCtrl)))
	{
		m_hardware.Request(Hardware::Req::DEBUG_RECORDER_PLAY_REVERSE, { {"frames", 1}});
		m_stateCurrent = m_hardware.Request(Hardware::Req::DEBUG_RECORDER_GET_STATE_CURRENT)->at("states");
	}

	ImGui::SameLine();
	if (ImGui::Button(" + "))
	{
		m_hardware.Request(Hardware::Req::DEBUG_RECORDER_PLAY_FORWARD, { {"frames", 1} });
		m_stateCurrent = m_hardware.Request(Hardware::Req::DEBUG_RECORDER_GET_STATE_CURRENT)->at("states");
	}
	dev::PopStyleCompact();
}

void dev::RecorderWindow::CallbackUpdateData(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	m_stateRecorded = m_hardware.Request(
		Hardware::Req::DEBUG_RECORDER_GET_STATE_RECORDED)->at("states");

	m_stateCurrent = m_hardware.Request(
		Hardware::Req::DEBUG_RECORDER_GET_STATE_CURRENT)->at("states");
}