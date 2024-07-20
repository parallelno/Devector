#include "Reverse.h"
#include "Utils/Utils.h"


// UI thread. status will be handled during Update
void dev::Reverse::SetStatus(const int _status)
{
	m_status = _status;
}

void dev::Reverse::Update(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
	IO::State* _ioStateP, Display::State* _displayStateP)
{
	switch (m_status)
	{
	case STATUS_RESET:
		m_stateIdx = 0;
		m_stateLen = 0;
		m_frameNum = _displayStateP->frameNum;
		StoreState(*_cpuStateP, *_memStateP, *_ioStateP, *_displayStateP, 0); // 0 - because StoreState shoudn't increase m_stateIdx
		m_status = STATUS_UPDATE;
		break;

	case STATUS_UPDATE:
		if (_memStateP->debug.writeLen) StoreMemory(*_memStateP);
		if (m_frameNum != _displayStateP->frameNum)
		{
			StoreState(*_cpuStateP, *_memStateP, *_ioStateP, *_displayStateP);
			m_frameNum = _displayStateP->frameNum;
		}
		break;

	case STATUS_RESTORE:
		for (int i=0; i<10; i++) RestoreState(_cpuStateP, _memStateP, _ioStateP, _displayStateP);
		m_status = STATUS_UPDATE;
		break;
	};
}

void dev::Reverse::StoreMemory(const Memory::State& _memState)
{
	auto& state = m_states[m_stateIdx];
	
	for (int i = 0; i < _memState.debug.writeLen; i++)
	{
		state.memBeforeWrites.push_back(_memState.debug.beforeWrite[i]);
		state.globalAddrs.push_back(_memState.debug.writeGlobalAddr[i]);
	}
}

void dev::Reverse::StoreState(const CpuI8080::State& _cpuState, const Memory::State& _memState, 
	const IO::State& _ioState, const Display::State& _displayState, size_t increment)
{
	// prepare for the next state
	m_stateIdx = (m_stateIdx + increment) % STATES_LEN;
	m_stateLen = dev::Min(m_stateLen + 1, STATES_LEN);

	auto& nextState = m_states[m_stateIdx];

	nextState.cpuState = _cpuState;
	nextState.memState = _memState.update;
	nextState.ioState = _ioState;
	nextState.displayState = _displayState;
	nextState.memBeforeWrites.clear();
	nextState.globalAddrs.clear();

	//GetStatesSize();
}

// has to be called when Hardware stopped
void dev::Reverse::RestoreState(CpuI8080::State* _cpuStateP,
	Memory::State* _memStateP, IO::State* _ioStateP, Display::State* _displayStateP)
{
	if (m_stateLen < 1) return;

	auto& state = m_states[m_stateIdx];
	m_stateIdx = (m_stateIdx - 1) % STATES_LEN;
	m_stateLen = dev::Max(m_stateLen - 1, 0);

	*_cpuStateP = state.cpuState;
	_memStateP->update = state.memState;
	*_ioStateP = state.ioState;
	*_displayStateP = state.displayState;
	auto& ram = *(_memStateP->ramP);

	for (auto i = state.globalAddrs.size() - 1; i >= 0; i--)
	{
		GlobalAddr globalAddr = state.globalAddrs[i];
		uint8_t val = state.memBeforeWrites[i];
		ram[globalAddr] = val;
	}
}

void dev::Reverse::GetStatesSize()
{
	m_statesMemSize = 0;
	for (int i = 0; i < m_stateLen; i++)
	{
		auto& state = m_states[(m_stateIdx - i) % STATES_LEN];
		m_statesMemSize += sizeof(state);
		m_statesMemSize += state.memBeforeWrites.size();
		m_statesMemSize += state.globalAddrs.size() * sizeof(GlobalAddr);
	}
}