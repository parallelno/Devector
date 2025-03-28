#pragma once

#include <cstdint>
#include <atomic>
#include "utils/types.h"
#include "core/cpu_i8080.h"
#include "core/memory.h"
#include "core/io.h"
#include "core/display.h"
#include "core/fdc_wd1793.h"

namespace dev
{
	class Recorder
	{
	public:
		static constexpr int FRAMES_PER_SEC = 50;
		static constexpr int STATES_LEN = FRAMES_PER_SEC * 600;
		using MemUpdates = std::vector<uint8_t>;
		using GlobalAddrs = std::vector<GlobalAddr>;

		static constexpr int STATUS_RESET = 0;	// erase the data, stores the first state
		static constexpr int STATUS_UPDATE = 1;	// enables updating
		static constexpr int STATUS_RESTORE = 2; // restore the last state
		// file format version 
		static constexpr uint32_t VERSION = 1;
		// it checks only first 8 bits of a version
		static constexpr uint32_t VERSION_MASK = 0xff;

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
		void CleanMemUpdates(Display::State* _displayStateP);
		auto GetStateRecorded() const -> size_t { return m_stateRecorded; };
		auto GetStateCurrent() const -> size_t { return m_stateCurrent; };
		void Deserialize(const std::vector<uint8_t>& _data, 
			CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
			IO::State* _ioStateP, Display::State* _displayStateP);
		auto Serialize() const -> const std::vector<uint8_t>;

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
		uint32_t m_version = VERSION;
	};
}