#include "Audio.h"

#include "Core/TimerI8253.h"
//#include "ay.h"
//#if !defined(__ANDROID_NDK__) && !defined(__GODOT__)
#include "SDL3/SDL.h"
//#endif
//#include "wav.h"

//#include "resampler.h"

static int print_driver_info()
{
#if !defined(__ANDROID_NDK__) && !defined(__GODOT__)
	/* Print available audio drivers */
	int n = SDL_GetNumAudioDrivers();
	if (n == 0) {
		SDL_Log("No built-in audio drivers\n\n");
	} else {
		int i;
		SDL_Log("Built-in audio drivers:\n");
		for (i = 0; i < n; ++i) {
			SDL_Log("  %d: %s\n", i, SDL_GetAudioDriver(i));
		}
		SDL_Log("Select a driver with the SDL_AUDIODRIVER environment variable.\n");
	}

	SDL_Log("Using audio driver: %s\n\n", SDL_GetCurrentAudioDriver());
#endif
	return 1;
}

void dev::Audio::init(/*WavRecorder* _rec*/)
{
	/*this->rec = _rec;

	if (Options.nosound) {
		return;
	}
	*/
#if !defined(__ANDROID_NDK__) && !defined(__GODOT__)
	SDL_AudioSpec want, have;
	SDL_memset(&want, 0, sizeof(want));
	want.freq = 48000;
	want.format = SDL_AUDIO_F32LE;//old AUDIO_F32;
	want.channels = 2;

	//if (!Options.vsync) {
		this->sound_frame_size = want.freq / 50;
	//}

	//want.samples = this->sound_frame_size;
	//want.callback = Audio::Callback;
	//want.userdata = (void *)this;

	SDL_Init(SDL_INIT_AUDIO);

	//Options.log.audio && ::print_driver_info();

	if(!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
		//fprintf(stderr, "SDL audio error: SDL_INIT_AUDIO not initialized\n");
		//return;
	}

	//if ((this->audiodev = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_OUTPUT, &want, Audio::Callback, (void*)this) == 0)
	//{
	//	//fprintf(stderr, "SDL audio error: %s", SDL_GetError());
	//	//Options.nosound = true;
	//	//return;
	//};

	//if (have.samples == this->sound_frame_size / 2) {
	//	// strange thing but we get a half buffer, try to get 2x
	//	SDL_CloseAudioDevice(this->audiodev);

	//	Options.log.audio &&
	//	fprintf(stderr, "SDL audio: retrying to open device with 2x buffer size\n");
	//	want.samples = this->sound_frame_size * 2;

	//	if ((this->audiodev = SDL_OpenAudioDevice(NULL, 0, &want, &have,
	//					SDL_AUDIO_ALLOW_FORMAT_CHANGE)) == 0) {
	//		fprintf(stderr, "SDL audio error: %s", SDL_GetError());
	//		Options.nosound = true;
	//		return;
	//	};

	//	if (have.samples < this->sound_frame_size) {
	//		fprintf(stderr, "SDL audio cannot get the right buffer size, giving up\n");
	//		Options.nosound = true;
	//	}
	//}

	this->sampleRate = have.freq;

	// one second = 50 frames
	// raster time in 12 MHz pixelclocks = 768 columns by 312 lines
	// timer clocks = pixel clock / 8
	int timer_cycles_per_second = 50*768*312/8; // 1497600
	// for 48000: 3120
	// for 44100: 3396, which gives 44098.9...
	this->sound_accu_top = (int)(0.5 + 100.0 * timer_cycles_per_second / this->sampleRate);

	/*Options.log.audio &&
	printf("SDL audio dev: %d, sample rate=%d "
			"have.samples=%d have.channels=%d have.format=%d have.size=%d\n",
			this->audiodev, this->sampleRate,
			have.samples, have.channels, have.format, have.size);*/

	/*
	// filters
	if (Options.nofilter) {
		resampler.set_passthrough(true);
	}
	*/
#else
	this->sampleRate = 48000;
	this->sound_frame_size = this->sampleRate / 50;
	int timer_cycles_per_second = 50*768*312/8; // 1497600
	this->sound_accu_top = (int)(0.5 + 100.0 * timer_cycles_per_second / this->sampleRate);
	// filters
	if (Options.nofilter) {
		resampler.set_passthrough(true);
	}
#endif
}

void dev::Audio::SoundSteps(int nclk1m5, int tapeout, int covox, int tapein)
{
	/*
	covox = covox - 255;
	int ech0 = Options.enable.timer_ch0,
		ech1 = Options.enable.timer_ch1,
		ech2 = Options.enable.timer_ch2,
		aych0 = Options.enable.ay_ch0,
		aych1 = Options.enable.ay_ch1,
		aych2 = Options.enable.ay_ch2;

	for (int clk = 0; clk < nclk1m5; ++clk) {
		float ay = this->aywrapper.step2(2, aych0, aych1, aych2);

		// timerwrapper does the stepping of 8253, it must always be called
		float soundf = (this->timerwrapper.singlestep(ech0, ech1, ech2)) * Options.volume.timer
			+ (tapeout + tapein) * Options.volume.beeper
			+ Options.volume.covox * (covox/256.0f)
			+ Options.volume.ay * ay;

		if (!Options.nosound) {
			soundf = this->resampler.sample(soundf);

			if (resampler.egg) {
				resampler.egg = false;
				this->sample(std::clamp(soundf * Options.volume.global, -1.f,1.f));
			}
		}
	}
	*/
}

void dev::Audio::Pause()
{
	//if (!Options.nosound) {
//#if !defined(__ANDROID_NDK__) && !defined(__GODOT__)
		SDL_PauseAudioDevice(audiodev);
//#endif
  //  }
	wrptr = 0;
	rdbuf = 0;
	wrbuf = 0;
}

void dev::Audio::Callback(void * _userdata, uint8_t * _stream, int _len)
{
	auto audio = (Audio*)_userdata;
	auto fstream = (float*)_stream;

	if (audio->rdbuf == audio->wrbuf) {
		memcpy(_stream, audio->buffer[audio->rdbuf], audio->wrptr * sizeof(float));
		for (int i = audio->wrptr, end = audio->sound_frame_size * 2; i < end; ++i)
		{
			fstream[i] = audio->last_value;
		}
		/*
		Options.log.audio &&
			fprintf(stderr, "starve rdbuf=%d wrbuf=%d en manque=%d\n",
					audio->rdbuf, audio->wrbuf, audio->sound_frame_size - audio->wrptr/2);
		*/
		audio->wrptr = 0;
	} else
	{
		memcpy(_stream, audio->buffer[audio->rdbuf], _len);
		if (++audio->rdbuf == Audio::NBUFFERS) {
			audio->rdbuf = 0;
		}
	}
/*
	audio->rec &&
		audio->rec->record_buffer(fstream, audio->sound_frame_size * 2);
*/

/*
#if !defined(__ANDROID_NDK__) && !defined(__GODOT__)
	// sound callback is also our frame interrupt source
	if (!(Options.vsync && Options.vsync_enable)) {
		extern uint32_t timer_callback(uint32_t interval, void * param);
		timer_callback(0, 0);
		DBG_QUEUE(putchar('s'); fflush(stdout););
	}	
#endif
*/
}

void dev::Audio::Sample(float samp)
{
//    if (!Options.nosound) {
#if !defined(__ANDROID_NDK__) && !defined(__GODOT__)
		//SDL_LockAudioDevice(audiodev);
#endif
		this->last_value = samp;
		this->buffer[this->wrbuf][this->wrptr++] = samp;
		this->buffer[this->wrbuf][this->wrptr++] = samp;
		if (this->wrptr >= this->sound_frame_size * 2) {
			this->wrptr = 0;
			if (++this->wrbuf == Audio::NBUFFERS) {
				this->wrbuf = 0;
			}
		}
#if !defined(__ANDROID_NDK__) && !defined(__GODOT__)
		//SDL_UnlockAudioDevice(audiodev);
#endif
//    }
}

void dev::Audio::reset()
{
	//this->aywrapper.reset();
}
