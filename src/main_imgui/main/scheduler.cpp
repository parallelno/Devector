#include "scheduler.h"

auto dev::Scheduler::AddSignal(Receiver&& _receiver)
-> uint64_t
{
	m_receivers.emplace_back(std::move(_receiver));
	return m_id++;
}


void dev::Scheduler::Update(dev::Hardware& _hardware, Debugger& _debugger)
{
	// it's called by UI loop, so always set UI_DRAW
	m_activeSignals = Signals::UI_DRAW;

	bool isRunning = _hardware.Request(
		Hardware::Req::IS_RUNNING)->at("isRunning");

	uint64_t cc = _hardware.Request(Hardware::Req::GET_CC)->at("cc");
	bool cc_updated = m_cc != cc;
	m_cc = cc;


	size_t bpUpdates = _hardware.Request(
		Hardware::Req::DEBUG_BREAKPOINT_GET_UPDATES)->at("updates");
	bool bpUpdated = m_bpUpdates != bpUpdates;
	m_bpUpdates = bpUpdates;


	size_t wpUpdates = _hardware.Request(
		Hardware::Req::DEBUG_WATCHPOINT_GET_UPDATES)->at("updates");
	bool wpUpdated = m_wpUpdates != wpUpdates;
	m_wpUpdates = wpUpdates;


	auto frameNum = _hardware.Request(
		Hardware::Req::GET_DISPLAY_DATA)->at("frameNum");
	bool frameNumUpdated = m_frameNum != frameNum;
	m_frameNum = frameNum;


	// detect HW_RUNNING signal
	m_activeSignals = (Signals)(m_activeSignals |
					(isRunning ? Signals::HW_RUNNING : Signals::NONE));
	// detect HW_START signal
	m_activeSignals = (Signals)(m_activeSignals |
						(isRunning && cc_updated ?
						Signals::START : Signals::NONE));
	// detect BREAK signal
	m_activeSignals = (Signals)(m_activeSignals |
						(!isRunning && cc_updated?
						Signals::BREAK : Signals::NONE));

	// detect BREAKPOINTS signal
	m_activeSignals = (Signals)(m_activeSignals |
						(bpUpdated ?
						Signals::BREAKPOINTS : Signals::NONE));

	// detect WATCHPOINTS signal
	m_activeSignals = (Signals)(m_activeSignals |
						(wpUpdated ?
						Signals::WATCHPOINTS : Signals::NONE));


	// detect FRAME signal
	m_activeSignals = (Signals)(m_activeSignals |
						(frameNumUpdated ?
						Signals::FRAME : Signals::NONE));

	for (auto& receiver : m_receivers){
		receiver.TryCall(m_activeSignals);
	}

	m_activeSignals = Signals::NONE;
}

void dev::Scheduler::Receiver::TryCall(const Signals _activeSignals)
{
	if ((flags & _activeSignals) == Signals::NONE ||
		!active)
		{
			return;
		}

	if (_activeSignals & Signals::HW_RUNNING &&
		hw_running_delay > 0ms)
		{
		auto now = std::chrono::steady_clock::now();
		if (now - lastTimePoint < hw_running_delay) {
			return;
		}
		lastTimePoint = now;
	}

	func(_activeSignals);
}
