#include "fft_audio.h"
#include <stdlib.h>
#include <float.h>
#include <assert.h>
#include <string.h>
#include "common.h"


#define SILENCE_VALUE	((float)0x0000)
#define NORM_VALUE		((float)0x8000)


void fft_audio_fill_window_data_rectangular(fft_audio * audio, size_t N)
{
	size_t i;

	for (i = 0; i < N; ++i) {
		audio->window_data[i] = 1.0f;
	}
}

void fft_audio_fill_window_data_welch(fft_audio * audio, size_t N)
{
	size_t i;
	float val;

	for (i = 0; i < N; ++i) {
		val = (i - 0.5f * (N - 1)) / (0.5f * N + 1);
		audio->window_data[i] = 1.0f - val * val;
	}
}

void fft_audio_fill_window_data_triangular(fft_audio * audio, size_t N)
{
	size_t i;
	const float val = N / 2.0f;

	for (i = 0; i < N; ++i) {
		audio->window_data[i] = (val - fabsf(i - val)) / val;
	}
}

void fft_audio_fill_window_data_barlett(fft_audio * audio, size_t N)
{
	size_t i;
	const float val = (N - 1) / 2.0f;

	for (i = 0; i < N; ++i) {
		audio->window_data[i] = (val - fabsf(i - val)) / val;
	}
}

void fft_audio_fill_window_data_hanning(fft_audio * audio, size_t N)
{
	size_t i;
	float val;

	for (i = 0; i < N; ++i) {
		val = cosf(2 * M_PI * i / (N - 1));
		audio->window_data[i] = 0.5f * (1.0f - val);
	}
}

void fft_audio_fill_window_data_hamming(fft_audio * audio, size_t N)
{
	size_t i;
	float val;

	for (i = 0; i < N; ++i) {
		val = cosf(2 * M_PI * i / (N - 1));
		audio->window_data[i] = 0.53836f - 0.46164f * val;
	}
}

void fft_audio_fill_window_data_blackman(fft_audio * audio, size_t N)
{
	size_t i;
	float val_one;
	float val_two;

	for (i = 0; i < N; ++i) {
		val_one = cosf(2 * M_PI * i / (N - 1));
		val_two = cosf(4 * M_PI * i / (N - 1));
		audio->window_data[i] = 0.42f - 0.5f * val_one + 0.8f * val_two;
	}
}

void fft_audio_fill_window_data(fft_audio * audio,
								const enum fft_audio_window windowing)
{
	size_t N = audio->window_elements;

	switch (windowing) {
		case fft_audio_rectangular:
			fft_audio_fill_window_data_rectangular(audio, N);
			break;
		case fft_audio_welch:
			fft_audio_fill_window_data_welch(audio, N);
			break;
		case fft_audio_triangular:
			fft_audio_fill_window_data_triangular(audio, N);
			break;
		case fft_audio_barlett:
			fft_audio_fill_window_data_barlett(audio, N);
			break;
		case fft_audio_hanning:
			fft_audio_fill_window_data_hanning(audio, N);
			break;
		case fft_audio_hamming:
			fft_audio_fill_window_data_hamming(audio, N);
			break;
		case fft_audio_blackman:
			fft_audio_fill_window_data_hamming(audio, N);
			break;
		default:
			fft_audio_fill_window_data_rectangular(audio, N);
			break;
	}
}

int fft_audio_init(fft_audio * audio,
				   const char filename[],
				   const size_t window_elements,
				   const enum fft_audio_window windowing)
{
	size_t i;
	SF_INFO info;

	assert(audio != NULL);
	assert(filename != NULL);
	assert(window_elements <= MAX_WINDOW_ELEMENTS);

	memset(&info, 0, sizeof(info));

	audio->file = sf_open(filename, SFM_READ, &info);

	if (audio->file == NULL) {
		DLOG("Cannot open file %s\n", filename);
		return FFT_AUDIO_ERROR_OPEN_FILE;
	}

	for (i = 0 ; i < MAX_DATA_ELEMENTS; ++i) {
		audio->data[i] = SILENCE_VALUE;
	}

	for (i = 0; i < MAX_WINDOW_ELEMENTS; ++i) {
		audio->fft_in[i][0] = SILENCE_VALUE;
		audio->fft_in[i][1] = 0.0f;
		audio->fft_out[i][0] = 0.0f;
		audio->fft_out[i][1] = 0.0f;
	}

	audio->samplerate		= info.samplerate;
	audio->channels			= info.channels;
	audio->frames			= info.frames;
	audio->window_elements	= window_elements;

	fft_audio_fill_window_data(audio, windowing);

	audio->plan = fftwf_plan_dft_1d(window_elements,
									audio->fft_in,
									audio->fft_out,
									FFTW_FORWARD,
									FFTW_ESTIMATE);

	return FFT_AUDIO_SUCCESS;
}

int fft_audio_read_data(fft_audio * audio,
						const size_t data_size)
{
	size_t readcount;
	size_t i;
	float aux;

	assert(data_size * audio->channels <= MAX_DATA_ELEMENTS);

	readcount = sf_read_float(audio->file, audio->data, data_size * audio->channels);
	if (readcount == 0) {
		DLOG("EOF", NULL);
		return FFT_AUDIO_EOF;
	}

	if (audio->channels == 1) {
		for (i = 0; i < readcount; ++i) {
			audio->fft_in[i][0] = audio->data[i] * NORM_VALUE;
		}
	} else {
		for (i = 0; i < readcount / 2; ++i) {
			aux = audio->data[i * 2] + audio->data[i * 2 + 1];
			audio->fft_in[i][0] = aux * NORM_VALUE;
		}
	}

	// Set all the remaining values to 0 if needed
	for (i = readcount; i < data_size; ++i) {
		audio->fft_in[i][0] = 0.0f;
	}

	// Apply window
	for (i = 0; i < data_size; ++i) {
		audio->fft_in[i][0] *= audio->window_data[i];
	}

	return FFT_AUDIO_SUCCESS;
}

int fft_audio_next_window(fft_audio * audio)
{
	int ret;

	assert(audio != NULL);

	ret = fft_audio_read_data(audio, audio->window_elements);
	if (ret == FFT_AUDIO_EOF) {
		return FFT_AUDIO_EOF;
	}

	fftwf_execute(audio->plan);
	fft_audio_stats_samples(&(audio->stats), audio, 1, audio->window_elements);

	return FFT_AUDIO_SUCCESS;
}

int fft_audio_stats_samples(fft_audio_stats * stats,
							const fft_audio * audio,
							const size_t from_sample,
							const size_t to_sample)
{
	size_t i;
	size_t samples = to_sample - from_sample;

	float real;
	float imag;
	float mag = 0.0f;
	float magMin = FLT_MAX;
	float magAvg = 0.0f;
	float magMax = FLT_MIN;
	float realSum = 0.0f;
	float imagSum = 0.0f;

	assert(stats != NULL);
	assert(audio != NULL);
	assert(from_sample <= audio-> window_elements);
	assert(to_sample <= audio->window_elements);

	for (i = from_sample; i < to_sample; ++i) {
		real = audio->fft_out[i][0];
		imag = audio->fft_out[i][1];

		mag = real * real + imag * imag;

		magMin = MIN(magMin, mag);
		magAvg += mag;
		magMax = MAX(magMax, mag);

		realSum += real;
		imagSum += imag;
	}

	stats->magMin = magMin;
	stats->magAvg = magAvg / samples;
	stats->magMax = magMax;

	stats->realSum = realSum;
	stats->imagSum = imagSum;
	stats->amplitude = 2 * sqrtf(realSum * realSum + imagSum * imagSum) / samples;
	stats->RMS = sqrtf(2) / 2.0f * stats->amplitude;
	stats->dB = 20 * log10f(stats->amplitude);
	stats->phase = atanf(imagSum / realSum);

	return FFT_AUDIO_SUCCESS;
}

int fft_audio_free(fft_audio * audio)
{
	assert(audio != NULL);

	if (audio->file != NULL) {
		sf_close(audio->file);
	}

	if (audio->plan != NULL){
		fftwf_destroy_plan(audio->plan);
	}

	return FFT_AUDIO_SUCCESS;
}
