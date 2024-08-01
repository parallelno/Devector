#include "Recorder.h"
#include "Utils/Utils.h"

void dev::Recorder::Reset(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
	IO::State* _ioStateP, Display::State* _displayStateP)
{
	m_stateIdx = m_stateRecorded = m_stateCurrent = 0;
	m_lastRecord = true;
	m_frameNum = _displayStateP->update.frameNum; 
	StoreState(*_cpuStateP, *_memStateP, *_ioStateP, *_displayStateP);
	
	// store initial ram
	const auto& ram = *(_memStateP->ramP);
	m_ram = ram;
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
	if (m_frameNum != _displayStateP->update.frameNum)
	{
		StoreState(*_cpuStateP, *_memStateP, *_ioStateP, *_displayStateP);
		m_frameNum = _displayStateP->update.frameNum;
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
	nextState.displayState = _displayState.update;
	nextState.memBeforeWrites.clear();
	nextState.memWrites.clear();
	nextState.globalAddrs.clear();
}

void dev::Recorder::PlayForward(const int _frames, CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
	IO::State* _ioStateP, Display::State* _displayStateP)
{
	for (int i = 0; i < _frames; i++)
	{
		
		if (m_stateCurrent == m_stateRecorded)
		{
			m_lastRecord = false; // we play forward only to the start of the frame
			break;
		}

		// restore the memory of the current state
		auto& state = m_states[m_stateIdx];
		auto& ram = *(_memStateP->ramP);

		for (int i = 0; i < state.globalAddrs.size(); i++)
		{
			GlobalAddr globalAddr = state.globalAddrs[i];
			uint8_t val = state.memWrites[i];
			ram[globalAddr] = val;
		}

		m_stateIdx = (m_stateIdx + 1) % STATES_LEN;
		m_stateCurrent++;

		// restore the HW state of the next frame (+ one executed intruction)
		auto& stateNext = m_states[m_stateIdx];
		*_cpuStateP = stateNext.cpuState;
		_memStateP->update = stateNext.memState;
		*_ioStateP = stateNext.ioState;
		_displayStateP->update = stateNext.displayState;
	}

	_displayStateP->FrameBuffUpdate();
}

void dev::Recorder::PlayReverse(const int _frames, CpuI8080::State* _cpuStateP,
	Memory::State* _memStateP, IO::State* _ioStateP, Display::State* _displayStateP)
{
	for (int i = 0; i < _frames; i++)
	{
		if (m_stateCurrent == 1 && !m_lastRecord) {
			break;
		}

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
		_displayStateP->update = state.displayState;
		auto& ram = *(_memStateP->ramP);

		for (int i = state.globalAddrs.size() - 1; i >= 0; i--)
		{
			GlobalAddr globalAddr = state.globalAddrs[i];
			uint8_t val = state.memBeforeWrites[i];
			ram[globalAddr] = val;
		}
	}

	_displayStateP->FrameBuffUpdate();
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

// on loads
// requires Reset() after calling it
void dev::Recorder::Deserialize(const std::vector<uint8_t>& _data)
{
	size_t dataOffset = 0;
	// format version
	uint8_t version = _data[0];
	dataOffset++;
	if (version & VERSION) return;

	// global memory
	std::copy(_data.begin() + dataOffset, _data.begin() + Memory::GLOBAL_MEMORY_LEN, m_ram.begin());
	dataOffset += Memory::GLOBAL_MEMORY_LEN;

	m_stateRecorded = *(size_t*)(&_data[1]);
	dataOffset += sizeof(m_stateRecorded);
	m_stateIdx = 0;
	m_stateCurrent = 1;

	// states
	int firstStateIdx = 0;

	for (int stateIdx = firstStateIdx; stateIdx < firstStateIdx + m_stateRecorded; stateIdx++)
	{
		auto& state = m_states[stateIdx % STATES_LEN];

		state.cpuState = *(CpuI8080::State*)(&_data[dataOffset]);
		dataOffset += sizeof(CpuI8080::State);

		state.memState = *(Memory::Update*)(&_data[dataOffset]);
		dataOffset += sizeof(Memory::Update);

		state.ioState = *(IO::State*)(&_data[dataOffset]);
		dataOffset += sizeof(IO::State);

		state.displayState = *(Display::Update*)(&_data[dataOffset]);
		dataOffset += sizeof(Display::Update);

		state.memWrites = *(std::vector<uint8_t>*)(&_data[dataOffset]);
		dataOffset += state.memBeforeWrites.size();

		// amount of mem updates
		int memUpdates = *(int*)(&_data[dataOffset]);
		dataOffset += sizeof(memUpdates);

		// mem updates
		state.memWrites.clear();
		state.memBeforeWrites.clear();
		state.globalAddrs.clear();
		
		state.memWrites.insert(state.memWrites.end(), _data.begin() + dataOffset, _data.begin() + dataOffset + memUpdates);
		dataOffset += memUpdates;

		state.memWrites.insert(state.memWrites.end(), _data.begin() + dataOffset, _data.begin() + dataOffset + memUpdates);
		dataOffset += memUpdates;

		state.memWrites.insert(state.memWrites.end(), _data.begin() + dataOffset, _data.begin() + dataOffset + memUpdates);
		dataOffset += memUpdates;
	}

}

// on save
auto dev::Recorder::Serialize() const
-> const std::vector<uint8_t>
{
	if (m_stateRecorded == 0) return {};

	std::vector<uint8_t> result;

	// format version
	result.push_back(VERSION);

	// global memory
	result.insert(result.end(), m_ram.begin(), m_ram.end());

	// state recorded
	result.insert(result.end(), reinterpret_cast<const uint8_t*>(&m_stateRecorded),
		reinterpret_cast<const uint8_t*>(&m_stateRecorded + 1));

	// states
	int firstStateIdx = (m_stateIdx - m_stateRecorded + 1) % STATES_LEN;

	for (int stateIdx = firstStateIdx; stateIdx < firstStateIdx + m_stateRecorded; stateIdx++)
	{
		const auto& state = m_states[stateIdx % STATES_LEN];

		result.insert(result.end(), reinterpret_cast<const uint8_t*>(&state.cpuState),
		 	reinterpret_cast<const uint8_t*>(&state.cpuState + 1));

		result.insert(result.end(), reinterpret_cast<const uint8_t*>(&state.memState),
			reinterpret_cast<const uint8_t*>(&state.memState + 1));

		result.insert(result.end(), reinterpret_cast<const uint8_t*>(&state.ioState),
			reinterpret_cast<const uint8_t*>(&state.ioState + 1));

		result.insert(result.end(), reinterpret_cast<const uint8_t*>(&state.displayState),
			reinterpret_cast<const uint8_t*>(&state.displayState + 1));

		// amount of mem updates
		int memUpdates = state.memWrites.size();
		result.insert(result.end(), reinterpret_cast<const uint8_t*>(&memUpdates),
		reinterpret_cast<const uint8_t*>(&memUpdates + 1));

		// mem updates
		result.insert(result.end(), state.memWrites.begin(), state.memWrites.end());
		result.insert(result.end(), state.memBeforeWrites.begin(), state.memBeforeWrites.end());
		result.insert(result.end(), state.globalAddrs.begin(), state.globalAddrs.end());
	}

	return result;
}
