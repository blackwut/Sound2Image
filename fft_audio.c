#include "fft_audio.h"
#include <assert.h>
#include <math.h>
#include <fftw3.h>
#include <sndfile.h>
#include <float.h>
#include <string.h>
#include "common.h"


#define MAX_SAMPLERATE					44100
#define MAX_CHANNELS					2
#define MAX_DATA_ELEMENTS				(MAX_CHANNELS * MAX_SAMPLERATE)
#define MAX_WINDOW_ELEMENTS				MAX_SAMPLERATE

#define SILENCE_VALUE	((float)0x0000)
#define NORM_VALUE		((float)0x8000)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct {
	fftwf_plan plan;
	SNDFILE * file;
	float data[MAX_DATA_ELEMENTS];
	float window_data[MAX_DATA_ELEMENTS];
	fftwf_complex fft_in[MAX_WINDOW_ELEMENTS];
	fftwf_complex fft_out[MAX_WINDOW_ELEMENTS];

	fft_audio_stats stats;

	size_t samplerate;
	size_t channels;
	size_t frames;
	size_t window_elements;
} fft_audio;

static fft_audio audio;

static void fft_audio_fill_window_data_rectangular()
{
	size_t i;
	size_t N = audio.window_elements;

	for (i = 0; i < N; ++i) {
		audio.window_data[i] = 1.0f;
	}
}

static void fft_audio_fill_window_data_welch()
{
	size_t i;
	const size_t N = audio.window_elements;
	float val;

	for (i = 0; i < N; ++i) {
		val = (i - 0.5f * (N - 1)) / (0.5f * N + 1);
		audio.window_data[i] = 1.0f - val * val;
	}
}

static void fft_audio_fill_window_data_triangular()
{
	size_t i;
	const size_t N = audio.window_elements;
	const float val = N / 2.0f;

	for (i = 0; i < N; ++i) {
		audio.window_data[i] = (val - fabsf(i - val)) / val;
	}
}

static void fft_audio_fill_window_data_barlett()
{
	size_t i;
	const size_t N = audio.window_elements;
	const float val = (N - 1) / 2.0f;

	for (i = 0; i < N; ++i) {
		audio.window_data[i] = (val - fabsf(i - val)) / val;
	}
}

static void fft_audio_fill_window_data_hanning()
{
	size_t i;
	const size_t N = audio.window_elements;
	float val;

	for (i = 0; i < N; ++i) {
		val = cosf(2 * M_PI * i / (N - 1));
		audio.window_data[i] = 0.5f * (1.0f - val);
	}
}

static void fft_audio_fill_window_data_hamming()
{
	size_t i;
	const size_t N = audio.window_elements;
	float val;

	for (i = 0; i < N; ++i) {
		val = cosf(2 * M_PI * i / (N - 1));
		audio.window_data[i] = 0.53836f - 0.46164f * val;
	}
}

static void fft_audio_fill_window_data_blackman()
{
	size_t i;
	const size_t N = audio.window_elements;
	float val_one;
	float val_two;

	for (i = 0; i < N; ++i) {
		val_one = cosf(2 * M_PI * i / (N - 1));
		val_two = cosf(4 * M_PI * i / (N - 1));
		audio.window_data[i] = 0.42f - 0.5f * val_one + 0.8f * val_two;
	}
}

static void fft_audio_fill_window_data(const enum fft_audio_window windowing)
{

	switch (windowing) {
		case fft_audio_rectangular:
			fft_audio_fill_window_data_rectangular();
			break;
		case fft_audio_welch:
			fft_audio_fill_window_data_welch();
			break;
		case fft_audio_triangular:
			fft_audio_fill_window_data_triangular();
			break;
		case fft_audio_barlett:
			fft_audio_fill_window_data_barlett();
			break;
		case fft_audio_hanning:
			fft_audio_fill_window_data_hanning();
			break;
		case fft_audio_hamming:
			fft_audio_fill_window_data_hamming();
			break;
		case fft_audio_blackman:
			fft_audio_fill_window_data_blackman();
			break;
		default:
			fft_audio_fill_window_data_rectangular();
			break;
	}
}

int fft_audio_init(const char filename[],
				   const size_t window_elements,
				   const enum fft_audio_window windowing)
{
	size_t i;
	SF_INFO info;

	assert(filename != NULL);
	assert(window_elements <= MAX_WINDOW_ELEMENTS);

	memset(&info, 0, sizeof(info));

	audio.file = sf_open(filename, SFM_READ, &info);

	if (audio.file == NULL) {
		return FFT_AUDIO_ERROR_OPEN_FILE;
	}

	for (i = 0 ; i < MAX_DATA_ELEMENTS; ++i) {
		audio.data[i] = SILENCE_VALUE;
	}

	for (i = 0; i < MAX_WINDOW_ELEMENTS; ++i) {
		audio.fft_in[i][0] = SILENCE_VALUE;
		audio.fft_in[i][1] = 0.0f;
		audio.fft_out[i][0] = 0.0f;
		audio.fft_out[i][1] = 0.0f;
	}

	audio.samplerate		= info.samplerate;
	audio.channels			= info.channels;
	audio.frames			= info.frames;
	audio.window_elements	= window_elements;

	fft_audio_fill_window_data(windowing);

	audio.plan = fftwf_plan_dft_1d(window_elements,
								   audio.fft_in,
								   audio.fft_out,
								   FFTW_FORWARD,
								   FFTW_ESTIMATE);

	return FFT_AUDIO_SUCCESS;
}

size_t fft_audio_get_samplerate()
{
	return audio.samplerate;
}

size_t fft_audio_get_channels()
{
	return audio.channels;
}

int fft_audio_read_data(const size_t data_size)
{
	size_t readcount;
	size_t i;
	float aux;

	assert(data_size * audio.channels <= MAX_DATA_ELEMENTS);

	readcount = sf_read_float(audio.file, audio.data, data_size * audio.channels);
	if (readcount == 0) {
		return FFT_AUDIO_EOF;
	}

	if (audio.channels == 1) {
		for (i = 0; i < readcount; ++i) {
			audio.fft_in[i][0] = audio.data[i] * NORM_VALUE;
		}
	} else {
		for (i = 0; i < readcount / 2; ++i) {
			aux = audio.data[i * 2] + audio.data[i * 2 + 1];
			audio.fft_in[i][0] = aux * NORM_VALUE;
		}
	}

	// Set all the remaining values to 0 if needed
	for (i = readcount; i < data_size; ++i) {
		audio.fft_in[i][0] = 0.0f;
	}

	// Apply window
	for (i = 0; i < data_size; ++i) {
		audio.fft_in[i][0] *= audio.window_data[i];
	}

	return FFT_AUDIO_SUCCESS;
}

int fft_audio_load_next_window()
{
	int ret;

	ret = fft_audio_read_data(audio.window_elements);
	if (ret == FFT_AUDIO_EOF) {
		return FFT_AUDIO_EOF;
	}

	return FFT_AUDIO_SUCCESS;
}

void fft_audio_compute_fft()
{
	fftwf_execute(audio.plan);
	audio.stats = fft_audio_get_stats_samples(1, audio.window_elements);
}

void fft_audio_fill_buffer_data(float * buffer, const size_t fragments)
{
	size_t i;

	assert(buffer != NULL);

	for (i = 0; i < fragments * audio.channels; ++i) {
		buffer[i] = audio.data[i];
	}
}

fft_audio_stats fft_audio_get_stats()
{
	return audio.stats;
}

fft_audio_stats fft_audio_get_stats_samples(const size_t from,
											const size_t to)
{
	size_t i;
	size_t samples = to - from;
	float real;
	float imag;
	float mag = 0.0f;
	float magMin = FLT_MAX;
	float magAvg = 0.0f;
	float magMax = FLT_MIN;
	fft_audio_stats stats;

	assert(from <= audio.window_elements);
	assert(to <= audio.window_elements);

	for (i = from; i < to; ++i) {
		real = audio.fft_out[i][0];
		imag = audio.fft_out[i][1];

		mag = real * real + imag * imag;

		magMin = MIN(magMin, mag);
		magAvg += mag;
		magMax = MAX(magMax, mag);
	}

	stats.magMin = magMin;
	stats.magAvg = magAvg / samples;
	stats.magMax = magMax;

	return stats;
}

void fft_audio_free()
{
	if (audio.file != NULL) {
		sf_close(audio.file);
	}

	if (audio.plan != NULL){
		fftwf_destroy_plan(audio.plan);
	}
}
