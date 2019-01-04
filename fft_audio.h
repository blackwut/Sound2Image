#ifndef FFT_AUDIO
#define FFT_AUDIO

#include <math.h>
#include <fftw3.h>

#define FFT_AUDIO_SUCCESS 0

#define MAX_SAMPLERATE  (44100)
#define MAX_SECONDS     (10)
#define MAX_BLOCK_SIZE   (MAX_SAMPLERATE * MAX_SECONDS)

typedef struct fft_audio_stats {
    float max;
    float min;
    float power;
    float amplitude;
    float phase;
} fft_audio_stats;

typedef struct fft_audio_block {
    fftwf_complex data[MAX_BLOCK_SIZE];
    size_t size;
    fft_audio_stats stats;

} fft_audio_block;


int fft_audio_init(float * data,
                   size_t data_size,
                   size_t samplerate,
                   size_t block_size);
int fft_audio_next_block(fft_audio_block * block);
int fft_audio_get_stats(fft_audio_stats * stats,
                        fft_audio_block * block,
                        size_t freq,
                        size_t size);
int fft_audio_free();

#endif
