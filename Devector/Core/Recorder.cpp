#include "Recorder.h"
#include "Utils/Utils.h"

void dev::Recorder::Reset(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
	IO::State* _ioStateP, Display::State* _displayStateP)
{
	m_stateIdx = m_stateRecorded = m_stateCurrent = static_cast<size_t>(-1);  // -1 because StoreState increments them
	m_lastRecord = true;
	m_frameNum = _displayStateP->frameNum; 
	StoreState(*_cpuStateP, *_memStateP, *_ioStateP, *_displayStateP);
}

// continue HW execution
void dev::Recorder::CleanMemUpdates()
{
	m_lastRecord = true;
	m_stateRecorded = m_stateCurrent;
	auto& state = m_states[m_stateIdx];

	state.memBeforeWrites.clear();
	state.memWrites.clear();
	state.globalAddrs.clear();
}

void dev::Recorder::Update(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
	IO::State* _ioStateP, Display::State* _displayStateP)
{	
	if (!m_lastRecord) CleanMemUpdates();

	if (_memStateP->debug.writeLen) StoreMemoryDiff(*_memStateP);
	if (m_frameNum != _displayStateP->frameNum)
	{
		StoreState(*_cpuStateP, *_memStateP, *_ioStateP, *_displayStateP);
		m_frameNum = _displayStateP->frameNum;
	}
}

void dev::Recorder::StoreMemoryDiff(const Memory::State& _memState)
{
	auto& state = m_states[m_stateIdx];
	
	for (int i = 0; i < _memState.debug.writeLen; i++)
	{
		state.memBeforeWrites.push_back(_memState.debug.beforeWrite[i]);
		state.memWrites.push_back(_memState.debug.write[i]);
		state.globalAddrs.push_back(_memState.debug.writeGlobalAddr[i]);
	}
}

void dev::Recorder::StoreState(const CpuI8080::State& _cpuState, const Memory::State& _memState, 
	const IO::State& _ioState, const Display::State& _displayState)
{
	// prepare for the next state
	m_stateIdx = (m_stateIdx + 1) % STATES_LEN;
	m_stateCurrent = dev::Min(m_stateCurrent + 1, STATES_LEN);
	m_stateRecorded = m_stateCurrent;

	auto& nextState = m_states[m_stateIdx];

	nextState.cpuState = _cpuState;
	nextState.memState = _memState.update;
	nextState.ioState = _ioState;
	nextState.displayState = _displayState;
	nextState.memBeforeWrites.clear();
	nextState.memWrites.clear();
	nextState.globalAddrs.clear();
}

void dev::Recorder::PlayForward(const int _frames, CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
	IO::State* _ioStateP, Display::State* _displayStateP)
{
	for (int i = 0; i < _frames; i++)
	{
		// restore the memory of the current state
		auto& state = m_states[m_stateIdx];
		auto& ram = *(_memStateP->ramP);

		for (int i = 0; i < state.globalAddrs.size(); i++)
		{
			GlobalAddr globalAddr = state.globalAddrs[i];
			uint8_t val = state.memWrites[i];
			ram[globalAddr] = val;
		}

		m_lastRecord = m_stateCurrent == m_stateRecorded;
		
		if (m_lastRecord) break;

		m_stateIdx = (m_stateIdx + 1) % STATES_LEN;
		m_stateCurrent++;

		// restore the HW state of the next frame (+ one executed intruction)
		auto& stateNext = m_states[m_stateIdx];
		*_cpuStateP = stateNext.cpuState;
		_memStateP->update = stateNext.memState;
		*_ioStateP = stateNext.ioState;
		*_displayStateP = stateNext.displayState;
	}

	//UpdateDisplay();
}

void dev::Recorder::PlayReverse(const int _frames, CpuI8080::State* _cpuStateP,
	Memory::State* _memStateP, IO::State* _ioStateP, Display::State* _displayStateP)
{
	for (int i = 0; i < _frames; i++)
	{
		if (m_stateCurrent < 1) break;

		if (m_lastRecord)
		{
			m_lastRecord = false;
		}
		else {
			m_stateIdx = (m_stateIdx - 1) % STATES_LEN;
			m_stateCurrent--;
		}

		// restore the HW state to the start of the frame + one executed intruction
		auto& state = m_states[m_stateIdx];
		*_cpuStateP = state.cpuState;
		_memStateP->update = state.memState;
		*_ioStateP = state.ioState;
		*_displayStateP = state.displayState;
		auto& ram = *(_memStateP->ramP);

		for (int i = state.globalAddrs.size() - 1; i >= 0; i--)
		{
			GlobalAddr globalAddr = state.globalAddrs[i];
			uint8_t val = state.memBeforeWrites[i];
			ram[globalAddr] = val;
		}
	}

	//UpdateDisplay();
}

void dev::Recorder::GetStatesSize()
{
	m_statesMemSize = 0;
	for (int i = 0; i < m_stateRecorded; i++)
	{
		auto& state = m_states[(m_stateIdx - i) % STATES_LEN];
		m_statesMemSize += sizeof(state);
		m_statesMemSize += state.memBeforeWrites.size();
		m_statesMemSize += state.globalAddrs.size() * sizeof(GlobalAddr);
	}
}