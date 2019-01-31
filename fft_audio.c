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

fftwf_plan _plan = NULL;
fftwf_complex _in[MAX_BLOCK_SIZE];
fftwf_complex _out[MAX_BLOCK_SIZE];

size_t _block_size;
size_t _block_offset;


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
    _block_size = block_size;
    _block_offset = 0;
    _plan = fftwf_plan_dft_1d(_block_size, _in, _out, FFTW_FORWARD, FFTW_ESTIMATE);

    return FFT_AUDIO_SUCCESS;
}

int fft_audio_next_block(fft_audio_block * block)
{
    assert(block != NULL);

    if (_block_offset + _block_size > _data_size) {
        return FFT_AUDIO_OUT_OF_SIZE;
    }

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
    assert(stats != NULL);
    assert(block != NULL);
    assert(size <= block->size);

    const size_t fromSample = FREQ_TO_SAMPLE(freq);
    const size_t toSample = FREQ_TO_SAMPLE(freq + size);
    float min          = DBL_MAX;
    float max          = DBL_MIN;
    float realSum      = 0;
    float imagSum      = 0;
    float power        = 0;
    float amplitude    = 0;
    float RMS          = 0;
    float dB           = 0;
    float phase        = 0;

    for (size_t i = fromSample; i < toSample; ++i) {
        const float real = block->data[i][0];
        const float imag = block->data[i][1];
        const float val = real * real + imag * imag;
        const float mag = sqrtf(val);

        if (mag > max) max = mag;
        if (mag < min) min = mag;
        realSum += real;
        imagSum += imag;
        power   += val;
    }

    amplitude   = 2 * sqrtf(power) / _block_size;
    RMS         = amplitude * (sqrt(2) / 2);
    dB          = 20 * log10(amplitude);
    phase       = atanf(imagSum / realSum);
    power       = power * 2;

    stats->min          = min;
    stats->max          = max;
    stats->power        = power;
    stats->amplitude    = amplitude;
    stats->RMS          = RMS;
    stats->dB           = dB;
    stats->phase        = phase;

    return FFT_AUDIO_SUCCESS;
}

int fft_audio_get_sub_block(fft_audio_block * sub_block,
                            const fft_audio_block * block,
                            const size_t freq,
                            const size_t size)
{
    assert(sub_block != NULL);
    assert(block != NULL);
    assert(size <= block->size);

    const size_t fromSample = FREQ_TO_SAMPLE(freq);
    const size_t toSample = FREQ_TO_SAMPLE(freq + size);

    for (size_t i = fromSample; i < toSample; ++i) {
        sub_block->data[i][0] = block->data[i][0];
        sub_block->data[i][1] = block->data[i][1];
    }

    sub_block->size = size;
    fft_audio_get_stats(&(sub_block->stats), sub_block, freq, size);

    return FFT_AUDIO_SUCCESS;
}

float magnitude_at_freq(fft_audio_block * block, size_t freq)
{
    const size_t fromSample = FREQ_TO_SAMPLE(freq - 1) + 1;
    const size_t toSample   = FREQ_TO_SAMPLE(freq);

    float val = 0;
    for (size_t i = fromSample; i < toSample; ++i) {
        const float real = block->data[i][0];
        const float imag = block->data[i][1];
        val += real * real + imag * imag;
    }

    val = sqrtf(val);

    return val;
}

int fft_audio_free() {
    if (_plan) fftwf_destroy_plan(_plan);
    return FFT_AUDIO_SUCCESS;
}
