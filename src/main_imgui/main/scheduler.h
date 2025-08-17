#pragma once

#include <string>
#include <chrono>
#include <functional>
#include <list>

#include "utils/types.h"

#include "core/hardware.h"
#include "core/debugger.h"

namespace dev
{
	/*
	* Scheduler
	* Allows to schedule functions calls from the main (UI) thread
	* based on the events in the hardware, debugger, or UI Draw calls.
	* Each function can be associated with one or more signals (events) and an
	* optional delay. If the signal is active, the function will be called
	* either on the next UI Draw call or after the specified delay has passed
	* since the last call. This is useful for updating UI elements in response
	* to changes in the hardware state without overwhelming the UI thread with
	* too many updates.
	*/
	class Scheduler
	{
		using Delay = std::chrono::duration<int64_t, std::milli>;

	public:
		using Clk = std::chrono::time_point<std::chrono::steady_clock>;

		enum Signals : uint32_t{
			NONE		= 0,
			HW_RESET	= 1 << 0,
			HW_RUNNING	= 1 << 1, // cpu is ticking
			START		= 1 << 2, // hardware starts after break
			BREAKPOINTS	= 1 << 3,
			WATCHPOINTS	= 1 << 4,
			UI_DRAW		= 1 << 5,
			BREAK		= 1 << 6,
			FRAME		= 1 << 7, // new frame started
		};
		using CallFunc = std::function<void(Signals)>;

		struct Receiver {
		public:
			Signals flags;
			CallFunc func;
			Delay hw_running_delay;
			bool& active;

			Receiver(const Signals _signals,
				CallFunc _func,
				bool& _active,
				const Delay hw_running_delay = 0ms)
				:
				flags{_signals}, func{std::move(_func)},
				active{_active},
				hw_running_delay{hw_running_delay},
				lastTimePoint{std::chrono::steady_clock::now() +
				Delay(int64_t(
					double(hw_running_delay.count()) * std::rand() / RAND_MAX)
					)}
				{}

			auto GetLastTimePoint() const -> const Clk& { return lastTimePoint; }
			void TryCall(const Signals _activeSignals);
		private:
			Clk lastTimePoint;
		};

		void Update(dev::Hardware& _hardware, Debugger& _debugger);
		auto AddSignal(Receiver&& _receiver) -> uint64_t;

	private:

		bool m_inited = false;
		uint64_t m_id = 0;
		uint64_t m_cc = 0;
		uint64_t m_bpUpdates = 0;
		uint64_t m_wpUpdates = 0;
		uint64_t m_frameNum = 0;
		Signals m_activeSignals = Signals::NONE;

		std::list<Receiver> m_receivers;
	};
}