#include "scheduler.h"

void dev::Scheduler::AddCallback(Callback&& _callback)
{
	m_callbacks.emplace_back(std::move(_callback));
}


void dev::Scheduler::AddSignal(PendingSignal&& _pendingSignals)
{
	m_pendingSignals.emplace_back(std::move(_pendingSignals));
}


void dev::Scheduler::Update(dev::Hardware& _hardware, Debugger& _debugger)
{
	// it's called by UI loop, so always set UI_DRAW signal
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

	// detect signals
	m_activeSignals |= isRunning ? Signals::HW_RUNNING : Signals::NONE;
	m_activeSignals |= isRunning && cc_updated ?Signals::START : Signals::NONE;
	m_activeSignals |= !isRunning && cc_updated ? Signals::BREAK : Signals::NONE;
	m_activeSignals |= bpUpdated ? Signals::BREAKPOINTS : Signals::NONE;
	m_activeSignals |= wpUpdated ? Signals::WATCHPOINTS : Signals::NONE;
	m_activeSignals |= frameNumUpdated ? Signals::FRAME : Signals::NONE;

	// send signals
	CallCallbacks(m_activeSignals);
	CallPendingSignals();
}


bool dev::Scheduler::CallCallbacks(const Signals _signals, SignalData _data)
{
	bool called = false;

	for (auto& callback : m_callbacks)
	{
		if (callback.activeP && !*callback.activeP) continue;

		if ((callback.flags & _signals) == Signals::NONE){
			continue;
		}

		// check active signals
		if (_signals & Signals::HW_RUNNING &&
			callback.hw_running_delay > 0ms)
		{
			auto now = std::chrono::steady_clock::now();
			if (now - callback.lastTimePoint < callback.hw_running_delay) {
				continue;
			}
			callback.lastTimePoint = now;
		}

		callback.func(_signals, _data);
		called = true;
	}

	return called;
}


void dev::Scheduler::CallPendingSignals()
{
	if (m_pendingSignals.empty()) return;

	for (auto it = m_pendingSignals.begin();
			it != m_pendingSignals.end();)
	{
		auto [flags, data, _] = *it;
		bool called = CallCallbacks(flags, data);
		if (called)
		{
			it->postponed--;

			if (it->postponed <= 0){
				it = m_pendingSignals.erase(it);
			}
			else{
				++it;
			}
		}
		else{
			++it;
		}
	}
}
