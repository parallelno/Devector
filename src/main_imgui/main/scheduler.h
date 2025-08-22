#pragma once

#include <string>
#include <chrono>
#include <functional>
#include <list>
#include <optional>
#include <variant>
#include <deque>

#include "signals.h"
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
		struct GlobalAddrLen {
			GlobalAddr globalAddr;
			uint16_t len;
		};
		using SignalData = std::optional<std::variant<
			GlobalAddr,
			GlobalAddrLen,
			bool>>;

		using Clk = std::chrono::time_point<std::chrono::steady_clock>;
		using CallFunc = std::function<void(const Signals, SignalData)>;

		struct Callback {
		public:
			CallFunc func;
			Delay hw_running_delay;
			Clk lastTimePoint;
			Signals flags;
			bool* activeP = nullptr;

			Callback(
				const Signals _signals,
				CallFunc _func,
				bool* _activeP = nullptr,
				const Delay hw_running_delay = 0ms)
				:
				flags{_signals}, func{std::move(_func)},
				activeP{_activeP},
				hw_running_delay{hw_running_delay},
				lastTimePoint{std::chrono::steady_clock::now() +
				Delay(int64_t(
					double(hw_running_delay.count()) * std::rand() / RAND_MAX)
					)}
				{}
		};

		struct PendingSignal {
			Signals signals = Signals::NONE;
			SignalData data = std::nullopt;
			int postponed = 1; // signal can be postponed this many times
		};

		void Update(dev::Hardware& _hardware, Debugger& _debugger);
		void AddCallback(Callback&& _callback);
		void AddSignal(PendingSignal&& _pendingSignals);
		void CallPendingSignals();

	private:
		bool CallCallbacks(
			const Signals _signals, SignalData _data = std::nullopt);

		uint64_t m_cc = 0;
		uint64_t m_bpUpdates = 0;
		uint64_t m_wpUpdates = 0;
		uint64_t m_frameNum = 0;
		Signals m_activeSignals = Signals::NONE;

		std::list<Callback> m_callbacks;
		std::deque<PendingSignal> m_pendingSignals;
	};
}