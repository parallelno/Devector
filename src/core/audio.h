#pragma once

#include <atomic>
#include <array>
#include "core/timer_i8253.h"
#include "core/sound_ay8910.h"
#include "SDL3/SDL.h"

namespace dev
{
    class Audio
    {
    private:
        static constexpr int INPUT_RATE = 1500000; // 1.5 MHz timer
        static constexpr int OUTPUT_RATE = 50000; // 50 KHz
        static constexpr int DOWNSAMPLE_RATE = INPUT_RATE / OUTPUT_RATE;
        static constexpr int CALLBACKS_PER_SEC = 100; // arbitrary number found while examining the SDL3 callback calls
        static constexpr int SDL_BUFFER = OUTPUT_RATE / CALLBACKS_PER_SEC; // the estimated SDL stream buff len
        static constexpr int SDL_BUFFERS = 8; // to make sure there is enough available data for audio streaming
        static constexpr int BUFFER_SIZE = SDL_BUFFER * SDL_BUFFERS;
        static constexpr int TARGET_BUFFERING = SDL_BUFFER * 4;
        static constexpr int LOW_BUFFERING = TARGET_BUFFERING - SDL_BUFFER * 2;
        static constexpr int HIGH_BUFFERING = TARGET_BUFFERING + SDL_BUFFER * 2;        

        TimerI8253& m_timer;
        AYWrapper& m_aywrapper;
        SDL_AudioDeviceID m_audioDevice = 0;
        SDL_AudioStream* m_stream = nullptr;
        float m_muteMul = 1.0f;

        std::array<float, BUFFER_SIZE> m_buffer; // Audio system writes to it, SDL reads from it
        std::atomic_uint64_t m_readBuffIdx = 0; // the last sample played by SDL
        std::atomic_uint64_t m_writeBuffIdx = 0; // the last sample stored by the Audio system
        std::atomic<float> m_lastSample = 0.0f;

        std::atomic_bool m_inited = false;
        std::atomic_int m_downsampleRate = DOWNSAMPLE_RATE;

        bool Downsample(float& _sample);

    public:
        Audio(TimerI8253& _timer, AYWrapper& _aywrapper);
        ~Audio();
        void Init();
        void Pause(bool _pause);
        void Mute(const bool _mute);
        static void Callback(void* _userdata, SDL_AudioStream* _stream, int _additionalAmount, int _totalAmount);
        void Clock(int _cycles, const float _beeper);
        void Reset();
    };

}