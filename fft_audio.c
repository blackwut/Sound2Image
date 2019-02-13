#include "fft_audio.h"
#include <stdlib.h>
#include <sndfile.h>
#include <float.h>
#include <assert.h>


// fftwf_plan plan = NULL;
// fftwf_complex in[MAX_WINDOW_SIZE];

// int fft_audio_init(fft_audio * audio,
// 				   const char filename[],
// 				   const size_t window_size)
// {
// 	SNDFILE * file;
// 	SF_INFO info;
// 	int i;

// 	assert(audio != NULL);
// 	assert(filename != NULL);

// 	memset(&info, 0, sizeof(info));

// 	file = sf_open(filename, SFM_READ, &info);

// 	if (file == NULL) {
// 		DLOG("Cannot open file %s\n", filename);
// 		return FFT_AUDIO_ERROR_OPEN_FILE;
// 	}

// 	audio->file = file;

// 	for (i = 0 ; i < MAX_DATA_SIZE; ++i) {
// 		data[i] = 0.0f;
// 	}

// 	for (i = 0; i < MAX_WINDOW_SIZE; ++i) {
// 		fft_data[i][0] = 0.0f;
// 		fft_data[i][1] = 0.0f;
// 	}

// 	audio->samplerate	= info.channels;
// 	audio->channels		= info.channels;
// 	audio->frames		= info.frames;
// 	audio->window_size	= window_size;

// 	plan = fftwf_plan_dft_1d(window_size,
// 							 in,
// 							 audio->fft_data,
// 							 FFTW_FORWARD,
// 							 FFTW_ESTIMATE);

// 	return FFT_AUDIO_SUCCESS;
// }

// int fft_audio_read_data(size_t data_size)
// {
// 	framesToRead = BUFFER_SIZE;
// 	while ((readcount = sf_read_float(infile, buffer, framesToRead)) > 0) {
// 		for (int k = 0; k < readcount; ++k) {
// 			data_play[dataplayid++] = buffer[k];
// 		}
// 	}
// }

// int fft_audio_free(fft_audio * audio)
// {
// 	assert(audio != NULL);

// 	if (audio->file != NULL) {
// 		sf_close(audio->file);
// 	}

// 	if (plan != NULL){
// 		fftwf_destroy_plan(plan);
// 	}
// }



// int fft_audio_fft_at_ms(fft_audio * audio,
// 						const size_t ms);



#define BEST_BLOCK_SIZE(b)  (((b) > _samplerate ? (b) : _samplerate + 1))


float * _data = NULL;
size_t _data_size;
size_t _samplerate;

fftwf_plan _plan = NULL;
fftwf_complex _in[MAX_BLOCK_SIZE];
fftwf_complex _out[MAX_BLOCK_SIZE];

size_t _block_size;
size_t _block_offset;


int fft_audio_init(float * data,
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

int fft_audio_block_shift_ms(fft_audio_block * block, const int shift_ms)
{
	assert(block != NULL);

	if (_block_offset + MS_TO_FRAMES(shift_ms) >= _data_size) {
		return FFT_AUDIO_ERROR_OUT_OF_SIZE;
	}

	for (size_t i = 0; i < _block_size; ++i) {
		_in[i][0] = _data[_block_offset + i];
		_in[i][1] = 0.f;
	}
	_block_offset += MS_TO_FRAMES(shift_ms);

	fftwf_execute(_plan);

	block->size = _block_size;
	for (size_t i = 0; i < _block_size; ++i) {
		block->data[i][0] = _out[i][0];
		block->data[i][1] = _out[i][1];
	}

	fft_audio_get_stats(&(block->stats), block, 0, _block_size);

	return FFT_AUDIO_SUCCESS;
}

int fft_audio_next_block(fft_audio_block * block)
{
	return fft_audio_block_shift_ms(block, 1000);
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
	float min          = FLT_MAX;
	float max          = FLT_MIN;
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
	dB          = 20 * log(amplitude);
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
	if (_data) free(_data);
	if (_plan) fftwf_destroy_plan(_plan);
	return FFT_AUDIO_SUCCESS;
}
