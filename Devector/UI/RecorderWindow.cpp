#include "RecorderWindow.h"

#include <format>
#include "Utils/ImGuiUtils.h"
#include "Utils/StrUtils.h"

dev::RecorderWindow::RecorderWindow(Hardware& _hardware, Debugger& _debugger,
	const float* const _fontSizeP, const float* const _dpiScaleP,
	ReqUI& _reqUI)
	:
	BaseWindow("Recorder", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_hardware(_hardware), m_debugger(_debugger), 
	m_reqUI(_reqUI)
{}

void dev::RecorderWindow::Update(bool& _visible)
{
	BaseWindow::Update();

	if (_visible && ImGui::Begin(m_name.c_str(), &_visible, ImGuiWindowFlags_NoCollapse))
	{
		bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");
		UpdateData(isRunning);
		Draw();

		ImGui::End();
	}
}

void dev::RecorderWindow::Draw()
{
	static int frame = 0;
	int framesLen = 3000;

	//ImGui::Text("Frames recorded: %d", framesLen);

	ImGui::Separator();

	if (ImGui::Button("Record") ||
		(ImGui::IsKeyPressed(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_R)))
	{
		m_ccLast = -1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Load"))
	{
		m_ccLast = -1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Save"))
	{
		m_ccLast = -1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		m_ccLast = -1;
	}

	ImGui::Dummy({ 1, 5 });

	dev::PushStyleCompact(0.5f);
	// Frame slider
	ImGui::SliderInt("##recTimeline", &frame, 1, framesLen, "%d", ImGuiSliderFlags_AlwaysClamp);

	ImGui::SameLine();
	if (ImGui::Button(" - "))
	{
		frame = dev::Max(1, frame - 1);
	}
	ImGui::SameLine();
	if (ImGui::Button(" + "))
	{
		frame = dev::Min(framesLen - 1, frame + 1);
	}
	dev::PopStyleCompact();
}

void dev::RecorderWindow::UpdateData(const bool _isRunning)
{
	// check if the hardware updated its state
	uint64_t cc = m_hardware.Request(Hardware::Req::GET_CC)->at("cc");
	auto ccDiff = cc - m_ccLast;
	//if (ccDiff == 0) return;
	m_ccLast = cc;

	// update
	
}