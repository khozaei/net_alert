#include "wav.h"
#include <portaudio.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>
#include <stdbool.h>

uv_rwlock_t play_lock;
bool _dismiss = false;
uv_timer_t timer;
int time_counter = 0;

static int
callback(const void *input, void *output,
		unsigned long frameCount,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData ){
	float *out;
	callback_data_s *p_data;
	sf_count_t num_read;

	out = (float*)output;
	p_data = (callback_data_s*)userData;

	/* clear output buffer */
	memset(out, 0, sizeof(float) * frameCount * p_data->info.channels);

	/* read directly into output buffer */
	num_read = sf_read_float(p_data->file, out, frameCount * p_data->info.channels);

	/*  If we couldn't read a full frameCount of samples we've reached EOF */
	if (num_read < frameCount)
	{
		return paComplete;
	}

	return paContinue;
}

static void
play(char *wav_path){
	SNDFILE *file;
	PaStream *stream;
	PaError error;
	callback_data_s data;
	
	/* Open the soundfile */
	data.file = sf_open(wav_path, SFM_READ, &data.info);
	if (sf_error(data.file) != SF_ERR_NO_ERROR)
	{
		fprintf(stderr, "%s\n", sf_strerror(data.file));
		fprintf(stderr, "File: %s\n", wav_path);
		return;
	}

	/* init portaudio */
	error = Pa_Initialize();
	if(error != paNoError)
	{
		fprintf(stderr, "Problem initializing\n");
		return;
	}

	/* Open PaStream with values read from the file */
	error = Pa_OpenDefaultStream(&stream
			      ,0                     /* no input */
			      ,data.info.channels         /* stereo out */
			      ,paFloat32             /* floating point */
			      ,data.info.samplerate
			      ,FRAMES_PER_BUFFER
			      ,callback
			      ,&data);        /* our sndfile data struct */
	if(error != paNoError)
	{
		fprintf(stderr, "Problem opening Default Stream\n");
		return;
	}

	/* Start the stream */
	error = Pa_StartStream(stream);
	if(error != paNoError)
	{
		fprintf(stderr, "Problem opening starting Stream\n");
		return;
	}

	/* Run until EOF is reached */
	while(Pa_IsStreamActive(stream))
	{
		Pa_Sleep(100);
	}

	/* Close the soundfile */
	sf_close(data.file);

	/*  Shut down portaudio */
	error = Pa_CloseStream(stream);
	if(error != paNoError)
	{
		fprintf(stderr, "Problem closing stream\n");
		return;
	}

	error = Pa_Terminate();
	if(error != paNoError)
	{
		fprintf(stderr, "Problem terminating\n");
		return;
	}
}

static void
alert(void *param){
	char *wav_path;

	wav_path = (char *)param;
	uv_rwlock_init(&play_lock);
	while (!_dismiss){
		play(wav_path);
		uv_sleep(1000);
	}
	uv_rwlock_destroy(&play_lock);
}

static void
timeout(uv_timer_t *t){
	printf("timer: counter= %i\n", time_counter);
	time_counter++;
	if (time_counter >= 35){
		time_counter = 0;
		dismiss();
		uv_timer_stop(t);
	}
}

void
do_alert(char *wav_path){
	uv_thread_t play_thread;

	uv_rwlock_init(&play_lock);
	_dismiss = false;
	uv_timer_init(uv_default_loop(), &timer);
	uv_timer_start(&timer, timeout, 1000, 1000);
	uv_rwlock_destroy(&play_lock);
	uv_thread_create(&play_thread, alert, wav_path);
}

void
dismiss(){
	uv_rwlock_init(&play_lock);
	_dismiss = true;
	uv_rwlock_destroy(&play_lock);
}
