#include "Audio.h"
#include <algorithm>
#include "Utils/Utils.h"

dev::Audio::Audio(TimerI8253& _timer, AYWrapper& _aywrapper) :
	m_timer(_timer), m_aywrapper(_aywrapper)
{
	Init();
}

dev::Audio::~Audio()
{
	Pause(true);
	SDL_DestroyAudioStream(m_stream);
}

void dev::Audio::Pause(bool _pause)
{
	if (_pause)
	{
		SDL_PauseAudioDevice(m_audioDevice);
	}
	else {
		SDL_ResumeAudioDevice(m_audioDevice);
	}
}

void dev::Audio::Mute(const bool _mute) { m_muteMul = _mute ? 0.0f : 1.0f; }

void dev::Audio::Reset()
{
	m_aywrapper.Reset();
	m_timer.Reset();
	m_buffer.fill(0);
	m_lastSample = m_readBuffIdx = m_writeBuffIdx = 0;
	m_muteMul = 1.0f;
}

void dev::Audio::Init()
{
	const SDL_AudioSpec spec = { SDL_AUDIO_F32, 1, 50000 };

	SDL_Init(SDL_INIT_AUDIO);

	if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
		dev::Log("SDL audio error: SDL_INIT_AUDIO not initialized\n");
		return;
	}

	m_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_OUTPUT, &spec, Callback, this);
	if (m_stream == NULL) {
		dev::Log("SDL_OpenAudioDeviceStream: the stream failed to create: %s\n", SDL_GetError());
	}

	m_audioDevice = SDL_GetAudioStreamDevice(m_stream);
	if (!m_audioDevice) {
		dev::Log("SDL_GetAudioStreamDevice: the device failed to create: %s\n", SDL_GetError());
	}
	SDL_ResumeAudioDevice(m_audioDevice);

	m_inited = true;
}

// _cycles are ticks of the 1.5 Mhz timer.
// Hardware thread
void dev::Audio::Clock(int _cycles, const float _beeper)
{
	if (!m_inited) return;

	//covox = covox - 255;

	for (int tick = 0; tick < _cycles; ++tick)
	{
		float sample = (m_timer.Clock(1) + m_aywrapper.Clock(2) + _beeper) * m_muteMul;

		if (Downsample(sample))
		{
			m_buffer[(m_writeBuffIdx++) % BUFFER_SIZE] = sample;
			m_lastSample = sample;
		}
	}
}


// resamples to a lower rate using a linear interpolation.
// returns true if the output sample is ready, false otherwise
bool dev::Audio::Downsample(float& _sample)
{
	static int sampleCounter = 0;
	static float accumulator = 0;
	accumulator += _sample;

	if (++sampleCounter > m_downsampleRate)
	{
		_sample = accumulator / m_downsampleRate;
		sampleCounter = 0;
		accumulator = 0;
		return true;
	}

	return false;
}

// feeds the SDL3 playback buffer.
void dev::Audio::Callback(void* _userdata, SDL_AudioStream* _stream, int _additionalAmount, int _totalAmount)
{
	if (_additionalAmount <= 0) return;

	Audio* audioP = (Audio*)_userdata;
	if (!audioP->m_inited) return;

	int buffering = audioP->m_writeBuffIdx - audioP->m_readBuffIdx;
	bool lowBuferring = buffering < SDL_BUFFER * 2;
	bool tooBuferring = buffering > SDL_BUFFER * 6;

	// malloc the SDL buff
	Uint8* data = SDL_stack_alloc(Uint8, _additionalAmount);
	if (!data) return;

	// convert the plain data to SDL_AUDIO_F32 buff;
	float* fstream = (float*)data;
	int fstreamLen = _additionalAmount / sizeof(float);

	if (lowBuferring)
	{
		// fill in with the lastSample when it's low buffering
		auto lastSample = audioP->m_lastSample.load();
		std::fill(fstream, fstream + fstreamLen, lastSample);

		auto floatResampleRate = --audioP->m_downsampleRate;
		dev::Log("SDL buffering is too low: {}. Sample rate is adjusted: {}", buffering, floatResampleRate);
	}
	else 
	{
		// copy the samples
		for (int i = 0; i < fstreamLen; i++)
		{
			fstream[i] = audioP->m_buffer[(audioP->m_readBuffIdx++) % BUFFER_SIZE];
		}

		if (tooBuferring)
		{
			audioP->m_readBuffIdx += fstreamLen;
			auto floatResampleRate = ++audioP->m_downsampleRate;
			dev::Log("SDL buffering is too big: {}. Sample rate is adjusted: {}", buffering, floatResampleRate);
		}
	}

	// memcopy the SDL buff
	SDL_PutAudioStreamData(_stream, data, _additionalAmount);
	SDL_stack_free(data);
}
