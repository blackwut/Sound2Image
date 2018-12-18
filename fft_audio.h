#ifndef FFT_AUDIO
#define FFT_AUDIO

#include <stdlib.h>

typedef struct fft_audio_block {
    float * data;
    size_t size;
    float max;
    float min;
    float power;
    float amplitude;
    float phase;
} fft_audio_block;

void fft_audio_init(float * data,
                    size_t data_size,
                    size_t samplerate,
                    size_t block_size);

fft_audio_block * fft_audio_next_block();

void fft_audio_block_free(fft_block * block);

void fft_audio_free();

#endif
