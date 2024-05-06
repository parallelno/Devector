#pragma once
#ifndef DEV_AUDIO_H
#define DEV_AUDIO_H

#include <array>
#include <inttypes.h>
#include <atomic>

#include "SDL3/SDL.h"
#include "Core/TimerI8253.h"

namespace dev
{
	class Audio
	{
	private:
		TimerWrapper& timerwrapper;
		//AYWrapper& aywrapper;
	#if !defined(__ANDROID_NDK__) && !defined(__GODOT__)
		SDL_AudioDeviceID audiodev;
	#endif
		static const int buffer_size = 2048 * 2; // 96000/50=1920, enough
		int sound_frame_size = 2048;

		static const int NBUFFERS = 8;
		float buffer[NBUFFERS][buffer_size];
		static const int mask = buffer_size - 1;
		std::atomic_int wrptr;
		int wrbuf;
		int rdbuf;
		float last_value;

		int sampleRate;

		int sound_accu_top;

		//Resampler resampler;
		//WavRecorder * rec;

	public:

		Audio(TimerWrapper & tw/*, AYWrapper& aw*/) : timerwrapper(tw)//,
			//aywrapper(aw)
		{}
		void init(/*WavRecorder* _rec = 0*/);

		void Pause();
		static void Callback(void* _userdata, uint8_t* _stream, int _len);
		void Sample(float samp);
		void SoundSteps(int nclk1m5, int tapeout, int covox, int tapein);

		void reset();
	};

}
#endif // !DEV_AUDIO_H