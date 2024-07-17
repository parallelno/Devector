#pragma once

#include <cstdint>
#include <atomic>
#include "Utils/Types.h"
#include "Core/CpuI8080.h"
#include "Core/Memory.h"
#include "Core/IO.h"
#include "Core/Display.h"
#include "Core/Fdc1793.h"

namespace dev
{
	class Reverse
	{
	public:
		static constexpr int FRAMES_PER_SEC = 50;
		static constexpr int STATES_LEN = FRAMES_PER_SEC * 60;
		using MemBeforeWrites = std::vector<uint8_t>;
		using GlobalAddrs = std::vector<GlobalAddr>;

		static constexpr int STATUS_RESET = 0;	// erase the data, stores the first state
		static constexpr int STATUS_UPDATE = 1;	// enables updating
		static constexpr int STATUS_RESTORE = 2; // restore the last state

#pragma pack(push, 1)
		struct State
		{
			CpuI8080::State cpuState;
			Memory::Update memState;
			MemBeforeWrites memBeforeWrites; // what was in memory before writes
			GlobalAddrs globalAddrs; // the global addresses where to restore memory
			IO::State ioState;
			Display::State displayState;
		};
#pragma pack(pop)

		using States = std::array<State, STATES_LEN>;

		void SetStatus(const int _status);
		void Update(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
			IO::State* _ioStateP, Display::State* _displayStateP);

	private:
		void StoreState(const CpuI8080::State& _cpuState, const Memory::State& _memState, 
			const IO::State& _ioState, const Display::State& _displayState);
		void StoreMemory(const Memory::State& _memState);
		void RestoreState(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
			IO::State* _ioStateP, Display::State* _displayStateP);
		void GetStatesSize();

		int m_stateIdx = 0; // points to the last stored state
		int m_stateLen = 0; // the amount of stored states
		States m_states;
		int m_frameNum = 0;
		std::atomic_int m_status = STATUS_RESET;
		int m_stateSize = 0;
	};
}