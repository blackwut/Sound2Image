#ifndef FFT_AUDIO_H
#define FFT_AUDIO_H

#include <math.h>
#include <fftw3.h>

#define FFT_AUDIO_SUCCESS				0
#define FFT_AUDIO_ERROR_OPEN_FILE		1
#define FFT_AUDIO_ERROR_OUT_OF_SIZE		2

#define MAX_SECONDS 1
#define MAX_BLOCK_SIZE 44100

#define MAX_SAMPLERATE		44100
#define MAX_WINDOW_SECONDS	1
#define MAX_WINDOW_SIZE		(MAX_SAMPLERATE * MAX_WINDOW_SECONDS)
#define MAX_DATA_SIZE 		(MAX_WINDOW_SIZE * 3)

#define FRAMES_TO_MS(f)     ((f) * 1000 / _samplerate)
#define MS_TO_FRAMES(ms)    ((ms) * _samplerate / 1000)
#define FREQ_TO_SAMPLE(f)   ((f) * (_block_size / (float)_samplerate))
#define SAMPLE_TO_FREQ(s)   ((s) * (_samplerate / (float)_block_size))

typedef struct fft_audio_stats {
	float min;
	float max;
	float power;
	float amplitude;
	float RMS;
	float dB;
	float phase;
} fft_audio_stats;

typedef struct fft_audio_block {
	fftwf_complex data[MAX_BLOCK_SIZE];
	size_t size;
	fft_audio_stats stats;
} fft_audio_block;

// typedef struct fft_audio {
// 	SNDFILE * file;
// 	float data[MAX_DATA_SIZE];
// 	fftwf_complex fft_data[MAX_WINDOW_SIZE];
// 	size_t samplerate;
// 	size_t channels;
// 	size_t frames;
// } fft_audio;

// int fft_audio_init(fft_audio * audio,
// 				   const char filename[],
// 				   const size_t window_size);

// int fft_audio_shift_ms(fft_audio * audio,
// 					   const size_t ms);

int fft_audio_init(float * data,
				   const size_t data_size,
				   const size_t samplerate,
				   const size_t block_size);
int fft_audio_block_shift_ms(fft_audio_block * block,
							 const int shift_ms);
int fft_audio_next_block(fft_audio_block * block);
int fft_audio_get_stats(fft_audio_stats * stats,
						const fft_audio_block * block,
						const size_t freq,
						const size_t size);
int fft_audio_get_sub_block(fft_audio_block * sub_block,
							const fft_audio_block * block,
							const size_t freq,
							const size_t size);

float magnitude_at_freq(fft_audio_block * block, size_t freq);

int fft_audio_free();

#endif
