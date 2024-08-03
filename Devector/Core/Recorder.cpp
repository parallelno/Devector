#include "Recorder.h"
#include "Utils/Utils.h"

void dev::Recorder::Reset(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
	IO::State* _ioStateP, Display::State* _displayStateP)
{
	m_stateIdx = m_stateRecorded = m_stateCurrent = 0;
	m_lastRecord = true;
	m_frameNum = _displayStateP->update.frameNum; 
	StoreState(*_cpuStateP, *_memStateP, *_ioStateP, *_displayStateP);
}

// continue HW execution
void dev::Recorder::CleanMemUpdates(Display::State* _displayStateP)
{
	m_lastRecord = true;
	m_stateRecorded = m_stateCurrent;
	auto& state = m_states[m_stateIdx];

	state.memBeforeWrites.clear();
	state.memWrites.clear();
	state.globalAddrs.clear();

	_displayStateP->BuffUpdate(Display::Buffer::BACK_BUFFER);
}

void dev::Recorder::Update(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
	IO::State* _ioStateP, Display::State* _displayStateP)
{	
	if (!m_lastRecord) CleanMemUpdates(_displayStateP);

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

	// store the ram
	m_ram = *_memState.ramP;
}

void dev::Recorder::RestoreState(CpuI8080::State* _cpuStateP, Memory::State* _memStateP,
	IO::State* _ioStateP, Display::State* _displayStateP)
{
	auto& state = m_states[m_stateIdx];

	*_cpuStateP = state.cpuState;
	_memStateP->update = state.memState;
	*_ioStateP = state.ioState;
	_displayStateP->update = state.displayState;

	// store the ram
	m_ram = *_memStateP->ramP;
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

	_displayStateP->BuffUpdate(Display::Buffer::FRAME_BUFFER);
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

	_displayStateP->BuffUpdate(Display::Buffer::FRAME_BUFFER);
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
void dev::Recorder::Deserialize(const std::vector<uint8_t>& _data, 
	CpuI8080::State* _cpuStateP, Memory::State* _memStateP, 
	IO::State* _ioStateP, Display::State* _displayStateP)
{
	size_t dataOffset = 0;
	// format version
	m_version = *(size_t*)(&_data[dataOffset]);
	dataOffset += sizeof(m_version);
	if ((m_version & VERSION_MASK) != VERSION) return;

	// ram
	std::copy(_data.begin() + dataOffset, _data.begin() + Memory::GLOBAL_MEMORY_LEN, m_ram.begin());
	dataOffset += Memory::GLOBAL_MEMORY_LEN;
	*_memStateP->ramP = m_ram;

	// m_stateRecorded
	m_stateRecorded = *(size_t*)(&_data[dataOffset]);
	dataOffset += sizeof(m_stateRecorded);
	
	// m_stateIdx
	m_stateIdx = m_stateRecorded - 1;
	
	// m_stateCurrent
	m_stateCurrent = *(size_t*)(&_data[dataOffset]);
	dataOffset += sizeof(m_stateCurrent);

	// m_lastRecord
	m_lastRecord = *(size_t*)(&_data[dataOffset]);
	dataOffset += sizeof(m_lastRecord);

	// states
	for (int stateIdx = 0; stateIdx < m_stateRecorded; stateIdx++)
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

		// amount of mem updates
		int memUpdates = *(int*)(&_data[dataOffset]);
		dataOffset += sizeof(memUpdates);
		if (memUpdates == 0) continue;

		// mem updates
		
		state.memWrites.assign(_data.begin() + dataOffset, _data.begin() + dataOffset + memUpdates);
		dataOffset += memUpdates;

		state.memBeforeWrites.assign(_data.begin() + dataOffset, _data.begin() + dataOffset + memUpdates);
		dataOffset += memUpdates;

		state.globalAddrs.assign(
			reinterpret_cast<const GlobalAddr*>(_data.data() + dataOffset),
			reinterpret_cast<const GlobalAddr*>(_data.data() + dataOffset + memUpdates * sizeof(GlobalAddr)));
		dataOffset += memUpdates * sizeof(GlobalAddr);
	}

	RestoreState(_cpuStateP, _memStateP, _ioStateP, _displayStateP);

	_displayStateP->BuffUpdate(Display::Buffer::FRAME_BUFFER);
	//_displayStateP->BuffUpdate(Display::Buffer::BACK_BUFFER);
}

// on save
auto dev::Recorder::Serialize() const
-> const std::vector<uint8_t>
{
	if (m_stateRecorded == 0) return {};

	std::vector<uint8_t> result;

	// format version
	result.insert(result.end(), reinterpret_cast<const uint8_t*>(&m_version),
		reinterpret_cast<const uint8_t*>(&m_version + 1));

	// ram
	result.insert(result.end(), m_ram.begin(), m_ram.end());

	// m_stateRecorded
	result.insert(result.end(), reinterpret_cast<const uint8_t*>(&m_stateRecorded),
		reinterpret_cast<const uint8_t*>(&m_stateRecorded + 1));

	// m_stateCurrent
	result.insert(result.end(), reinterpret_cast<const uint8_t*>(&m_stateCurrent),
		reinterpret_cast<const uint8_t*>(&m_stateCurrent + 1));

	// m_lastRecord
	result.insert(result.end(), reinterpret_cast<const uint8_t*>(&m_lastRecord),
		reinterpret_cast<const uint8_t*>(&m_lastRecord + 1));

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
		result.insert(result.end(),
			reinterpret_cast<const uint8_t*>(state.globalAddrs.data()),
			reinterpret_cast<const uint8_t*>(state.globalAddrs.data() + state.globalAddrs.size())
		);
	}

	return result;
}
