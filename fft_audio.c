#include "fft_audio.h"
#include <stdlib.h>
#include <float.h>
#include <assert.h>


#define BEST_BLOCK_SIZE(b) (((b) > _samplerate ? (b) : (_samplerate) + 1))
#define FREQ_TO_SAMPLE(f) ((f) * (_block_size / (float)_samplerate))
#define SAMPLE_TO_FREQ(s) ((s) * (_samplerate / (float)_block_size))

const float * _data;
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
int fft_audio_init(const float * data,
                   const size_t data_size,
                   const size_t samplerate,
                   const size_t block_size)
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
                        const fft_audio_block * block,
                        const size_t freq,
                        const size_t size)
{
    const size_t toFreq = freq + size;
    assert(toFreq <= MAX_SAMPLERATE);

    const size_t fromSample = FREQ_TO_SAMPLE(freq);
    const size_t toSample = FREQ_TO_SAMPLE(toFreq);
    double min = DBL_MAX;
    double max = DBL_MIN;
    double power = 0;
    double realSum = 0;
    double imagSum = 0;

    for (size_t i = fromSample; i < toSample; ++i) {
        const float real = block->data[j][0];
        const float imag = block->data[j][1];
        const float val = real * real + imag * imag;
        const float mag = sqrtf(val);

        if (mag > max) max = mag;
        if (mag < min) min = mag;
        power   += val;
        realSum += real;
        imagSum += imag;
    }

    stats->min          = min;
    stats->max          = max;
    stats->power        = power;
    stats->amplitude    = 2 * sqrt(power) / _block_size;
    stats->RMS          = amplitude * (sqrt(2) / 2);
    stats->dB           = atanf(imagSum / realSum);
    stats->phase        = 20 * log(stats->amplitude);

    return FFT_AUDIO_SUCCESS;
}

int fft_audio_free() {
    if (_plan) fftwf_destroy_plan(_plan); // TODO: check what fftwf_destroy_plan returns something
    return FFT_AUDIO_SUCCESS;
}
