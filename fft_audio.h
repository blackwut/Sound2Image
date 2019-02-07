#ifndef FFT_AUDIO_H
#define FFT_AUDIO_H

#include <math.h>
#include <fftw3.h>

#define FFT_AUDIO_SUCCESS 0
#define FFT_AUDIO_OUT_OF_SIZE 1

#define MAX_SAMPLERATE  (44100)
#define MAX_SECONDS     (1)
#define MAX_BLOCK_SIZE  (MAX_SAMPLERATE * MAX_SECONDS)

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


int fft_audio_init(const float * data,
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
