#include "scheduler.h"

auto dev::Scheduler::AddSignal(Receiver&& _receiver)
-> uint64_t
{
	m_receivers.emplace_back(std::move(_receiver));
	return m_id++;
}


void dev::Scheduler::Update(dev::Hardware& _hardware, Debugger& _debugger)
{
	m_activeSignals = Signals::UI_DRAW;
	bool isRunning = _hardware.Request(
		Hardware::Req::IS_RUNNING)->at("isRunning");

	m_activeSignals = (Signals)(m_activeSignals |
					(isRunning ? Signals::HW_RUNNING : Signals::NONE));

	m_activeSignals = (Signals)(m_activeSignals |
						(isRunning != m_isRunning ?
						Signals::RUN_PAUSE : Signals::NONE));

	m_activeSignals = (Signals)(m_activeSignals |
						(isRunning != m_isRunning && !isRunning?
						Signals::BREAK : Signals::NONE));

	m_isRunning = isRunning;


	for (auto& receiver : m_receivers)
	{
		receiver.TryCall(m_activeSignals);
	}
	m_activeSignals = Signals::NONE;
}

void dev::Scheduler::Receiver::TryCall(const Signals _activeSignals)
{
	if ((flags & _activeSignals) == Signals::NONE) return;

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
