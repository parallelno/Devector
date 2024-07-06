#pragma once

#include <atomic>
#include "TimerI8253.h"
//#include "ay.h"
#include "SDL3/SDL.h"

namespace dev
{
    class Audio
    {
    private:
        static constexpr int INPUT_RATE = 1500000; // 1.5 MHz timer
        static constexpr int OUTPUT_RATE = 50000; // 50 KHz
        static constexpr int DOWNSAMPLE_FACTOR = INPUT_RATE / OUTPUT_RATE;
        static constexpr int BUFFER_EXTRA = 48; // extra part to make sure there is enough data for audio streaming

        TimerI8253& m_timer;
        //AYWrapper& aywrapper;
        SDL_AudioDeviceID m_audioDevice;
        SDL_AudioStream* m_stream = nullptr;

        static constexpr size_t m_bufferSize = OUTPUT_RATE / 25 + BUFFER_EXTRA; // 25 callbacks per sec
        float m_buffer[m_bufferSize];
        std::atomic_uint m_writeBuffIdx = 0;
        float m_lastValue;

        bool Resampler(float& _sample);

    public:
        Audio(TimerI8253& _timer/*, AYWrapper& aw*/);
        ~Audio();
        void Init();
        void Pause(bool _pause);
        static void Callback(void* _userdata, SDL_AudioStream* _stream, int _additionalAmount, int _totalAmount);
        void Sample(float _sample);
        void Tick(int _ticks);
        void Reset();
    };

}