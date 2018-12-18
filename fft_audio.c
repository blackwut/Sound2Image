#include "fft_audio.h"
#include <float.h>
#include <math.h>
#include <fftw3.h>

float * _data = NULL;
size_t _data_size;
size_t _samplerate;

/* FFTW stuff */
fftwf_plan _plan = NULL;
fftwf_complex * _in;
fftwf_complex * _out;

/* block variables */
size_t _block_size;
size_t _block_offset;
size_t _bins_size;


/* Private functions */
size_t _fft_audio_best_block_size(size_t block_size) {
    return block_size + 1;
}

float _fft_audio_frequency(size_t i, float period, size_t n)
{
    return i / (float)(period * _block_size);
}

void _fft_audio_plan_free() {
    if (_plan) fftwf_destroy_plan(_plan);
}

void _fft_audio_in_free() {
    if (_in) fftwf_free(_in);
}

void _fft_audio_out_free() {
     if (_out) fftwf_free(_out);
}

/* Public functions */
void fft_audio_init(float * data,
                    size_t data_size,
                    size_t samplerate,
                    size_t block_size)
{
    _data = data;
    _data_size = data_size;
    _samplerate = samplerate;
    _bins_size = samplerate >> 1;
    _block_size = (block_size > samplerate) ? block_size : samplerate;
    _block_size = _fft_audio_best_block_size(_block_size);
    _block_offset = 0;

    _in = (fftwf_complex *)fftwf_malloc(_block_size * sizeof(fftwf_complex));
    _out = (fftwf_complex *)fftwf_malloc(_block_size * sizeof(fftwf_complex));
    _plan = fftwf_plan_dft_1d(_block_size, _in, _out, FFTW_FORWARD, FFTW_ESTIMATE);
}

fft_audio_block * fft_audio_next_block()
{
    if (_block_offset + _block_size > _data_size) return NULL;

    float * bins = (float *) calloc(_bins_size, sizeof(float));
    float max = FLT_MIN;
    float min = FLT_MAX;
    float power = 0;
    float realSum = 0;
    float imagSum = 0;

    for (size_t i = 0; i < _block_size; ++i) {
        _in[i][0] = _data[_block_offset + i];
        _in[i][1] = 0.f;
    }
    _block_offset += _block_size;

    fftwf_execute(_plan);

    const float period = 1.f / _samplerate;
    size_t j = 0;
    for (size_t i = 0; i < _bins_size; ++i) {
        float magSum = 0.f;
        size_t s = 0;
        while (j < _block_size >> 1 &&  _fft_audio_frequency(j, period, _block_size) <= i) {
            const float real = _out[j][0];
            const float imag = _out[j][1];
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

    fft_audio_block * block = (fft_block *) calloc(1, sizeof(fft_block));
    block->data = bins;
    block->size = _bins_size;
    block->max = max;
    block->min = min;
    block->power = power;
    block->amplitude = power / _block_size;
    block->phase = atanf(imagSum / realSum);

    return block;
}

void fft_audio_block_free(fft_block * block)
{
    free(block->data);
    free(block);
}

void fft_audio_free() {
    _fft_audio_plan_free();
    _fft_audio_in_free();
    _fft_audio_out_free();
}
