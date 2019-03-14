#ifndef FFT_AUDIO_H
#define FFT_AUDIO_H

#include <stdlib.h>

#define FFT_AUDIO_SUCCESS				0
#define FFT_AUDIO_ERROR_OPEN_FILE		1
#define FFT_AUDIO_EOF					2


enum fft_audio_window {
	fft_audio_rectangular,
	fft_audio_welch,
	fft_audio_triangular,
	fft_audio_barlett,
	fft_audio_hanning,
	fft_audio_hamming,
	fft_audio_blackman
};

typedef struct fft_audio_stats {
	float magMin;
	float magAvg;
	float magMax;
} fft_audio_stats;


int fft_audio_init(const char filename[],
				   const size_t window_elements,
				   const enum fft_audio_window windowing);
size_t fft_audio_get_samplerate();
size_t fft_audio_get_channels();
int fft_audio_load_next_window();
void fft_audio_compute_fft();
void fft_audio_fill_buffer_data(float * buffer, const size_t fragments);
fft_audio_stats fft_audio_get_stats();
fft_audio_stats fft_audio_get_stats_samples(const size_t from,
											const size_t to);
void fft_audio_free();

#endif
