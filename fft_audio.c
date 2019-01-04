#include "fft_audio.h"
#include <stdlib.h>
#include <float.h>
#include <assert.h>


#define BEST_BLOCK_SIZE(b) (b > _samplerate ? b : _samplerate) + 1
#define FREQUENCY_SAMPLE(i) (i * (_samplerate / (float)_block_size))

float * _data = NULL;
size_t _data_size;
size_t _samplerate;

/* FFTW stuff */
fftwf_plan _plan = NULL;
fftwf_complex _in[MAX_BLOCK_SIZE];
fftwf_complex _out[MAX_BLOCK_SIZE];

/* block variables */
size_t _block_size;
size_t _block_offset;
size_t _bins_size;


/* Public functions */
int fft_audio_init(float * data,
                    size_t data_size,
                    size_t samplerate,
                    size_t block_size)
{
    assert(data != NULL);
    assert(data_size > 0);
    assert(samplerate <= MAX_SAMPLERATE);
    assert(block_size <= MAX_BLOCK_SIZE);

    _data = data;
    _data_size = data_size;
    _samplerate = samplerate;
    _bins_size = samplerate / 2 ;
    _block_size = BEST_BLOCK_SIZE(block_size);
    _block_offset = 0;
    _plan = fftwf_plan_dft_1d(_block_size, _in, _out, FFTW_FORWARD, FFTW_ESTIMATE);

    return FFT_AUDIO_SUCCESS;
}

int fft_audio_next_block(fft_audio_block * block)
{
    if (_block_offset + _block_size > _data_size) return -1;

    for (size_t i = 0; i < _block_size; ++i) {
        _in[i][0] = _data[_block_offset + i];
        _in[i][1] = 0.f;
    }
    _block_offset += _block_size;

    fftwf_execute(_plan);

    block->size = _block_size;
    for (size_t i = 0; i < _block_size; ++i) {
        block->data[i][0] = _out[i][0];
        block->data[i][1] = _out[i][1];
    }

    fft_audio_get_stats(&(block->stats), block, 0, _samplerate);

    return FFT_AUDIO_SUCCESS;
}

int fft_audio_get_stats(fft_audio_stats * stats,
                        fft_audio_block * block,
                        size_t freq,
                        size_t size)
{
    assert((freq + size) <= MAX_SAMPLERATE);

    float bins[MAX_SAMPLERATE];
    float max = FLT_MIN;
    float min = FLT_MAX;
    float power = 0;
    float realSum = 0;
    float imagSum = 0;

    size_t j = 0;
    for (size_t i = freq; i < freq + size; ++i) {
        float magSum = 0.f;
        size_t s = 0;
        while (FREQUENCY_SAMPLE(j) <= i) {
            const float real = block->data[j][0];
            const float imag = block->data[j][1];
            const float val = real * real + imag * imag;
            const float mag = sqrtf(val);

            magSum += mag;
            s++;
            j++;

            if (mag > max) max = mag;
            if (mag < min) min = mag;
            power += val;
            realSum += real;
            imagSum += imag;
        }
        bins[i] = magSum / s;
    }

    stats->max = max;
    stats->min = min;
    stats->power = power;
    stats->amplitude = power / _block_size;
    stats->phase = atanf(imagSum / realSum);

    return FFT_AUDIO_SUCCESS;
}

int fft_audio_free() {
    if (_plan) fftwf_destroy_plan(_plan); // TODO: check what fftwf_destroy_plan returns
    return FFT_AUDIO_SUCCESS;
}
