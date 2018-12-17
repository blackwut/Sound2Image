#ifndef FFT_AUDIO
#define FFT_AUDIO

#include <stdlib.h>

void fft_audio_init(float * data,
                    size_t data_size,
                    size_t samplerate,
                    size_t block_size);

float * fft_audio_next_block();

void fft_audio_free();

#endif
