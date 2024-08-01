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
	class Recorder
	{
	public:
		static constexpr int FRAMES_PER_SEC = 50;
		static constexpr int STATES_LEN = FRAMES_PER_SEC * 60;
		using MemUpdates = std::vector<uint8_t>;
		using GlobalAddrs = std::vector<GlobalAddr>;

		static constexpr int STATUS_RESET = 0;	// erase the data, stores the first state
		static constexpr int STATUS_UPDATE = 1;	// enables updating
		static constexpr int STATUS_RESTORE = 2; // restore the last state
		static constexpr uint8_t VERSION = 1; // version of the file format

#pragma pack(push, 1)
		struct HwState
		{
			CpuI8080::State cpuState;
			Memory::Update memState;
			MemUpdates memBeforeWrites; // what was in memory before writes
			MemUpdates memWrites; // memory after writes
			GlobalAddrs globalAddrs; // the global addresses where to restore memory
			IO::State ioState;
			Display::Update displayState;
		};
#pragma pack(pop)

		using HwStates = std::array<HwState, STATES_LEN>; // one state per frame

		void Update(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
			IO::State* _ioStateP, Display::State* _displayStateP);
		void Reset(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
			IO::State* _ioStateP, Display::State* _displayStateP);
		void PlayForward(const int _frames, CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
			IO::State* _ioStateP, Display::State* _displayStateP);
		void PlayReverse(const int _frames, CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
			IO::State* _ioStateP, Display::State* _displayStateP);
		void CleanMemUpdates();
		auto GetStateRecorded() const -> size_t { return m_stateRecorded; };
		auto GetStateCurrent() const -> size_t { return m_stateCurrent; };
		void Deserialize(const std::vector<uint8_t>& _data); // on loads
		auto Serialize() const -> const std::vector<uint8_t>; // on save

	private:
		void StoreState(const CpuI8080::State& _cpuState, const Memory::State& _memState, 
			const IO::State& _ioState, const Display::State& _displayState);
		void StoreMemoryDiff(const Memory::State& _memState);
		void RestoreState(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
			IO::State* _ioStateP, Display::State* _displayStateP);
		void GetStatesSize();

		size_t m_stateIdx = 0; // idx of the last stored state in a circular buffer
		size_t m_stateRecorded = 0; // the amount of recorded states from 1 to STATES_LEN
		size_t m_stateCurrent = 0; // the number of current state from 1 to m_stateRecorded included
		bool m_lastRecord = true; // false means we at the end of recorded state + memory writes
		HwStates m_states;
		size_t m_statesMemSize = 0; // m_states memory consumption
		size_t m_frameNum = 0;
		Memory::Ram m_ram;
	};
}