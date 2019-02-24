#ifndef FFT_AUDIO_H
#define FFT_AUDIO_H

#include <math.h>
#include <fftw3.h>
#include <sndfile.h>

#define FFT_AUDIO_SUCCESS				0
#define FFT_AUDIO_EOF					1
#define FFT_AUDIO_ERROR_OPEN_FILE		2

#define MAX_SAMPLERATE					44100
#define MAX_CHANNELS					2
#define MAX_DATA_ELEMENTS				(MAX_CHANNELS * MAX_SAMPLERATE)
#define MAX_WINDOW_ELEMENTS				MAX_SAMPLERATE

#define FREQ_TO_SAMPLE(f)	((f) * audio->window_elements / audio->samplerate)
#define SAMPLE_TO_FREQ(s)	((s) * audio->samplerate / audio->window_elements)

typedef struct fft_audio_stats {

	float realSum;
	float imagSum;

	float magMin;
	float magAvg;
	float magMax;

	float amplitude;
	float RMS;
	float dB;
	float phase;
} fft_audio_stats;

typedef struct fft_audio {
	fftwf_plan plan;
	SNDFILE * file;
	float data[MAX_DATA_ELEMENTS];
	fftwf_complex fft_in[MAX_WINDOW_ELEMENTS];
	fftwf_complex fft_out[MAX_WINDOW_ELEMENTS];

	fft_audio_stats stats;

	size_t samplerate;
	size_t channels;
	size_t frames;
	size_t window_elements;
} fft_audio;

int fft_audio_init(fft_audio * audio,
				   const char filename[],
				   const size_t window_elements);

int fft_audio_next_window(fft_audio * audio);
int fft_audio_stats_samples(fft_audio_stats * stats,
							const fft_audio * audio,
							const size_t from_sample,
							const size_t to_sample);
// int fft_audio_stats_freq(fft_audio_stats * stats,
// 						const fft_audio * audio,
// 						const size_t from_freq,
// 						const size_t to_freq);
int fft_audio_free(fft_audio * audio);

#endif
