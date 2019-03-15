#include "fft_audio.h"
#include <sndfile.h>
#include <string.h>
#include <math.h>
#include <fftw3.h>
#include <float.h>
#include <assert.h>


//------------------------------------------------------------------------------
// FFT_AUDIO LOCAL CONSTANTS
//------------------------------------------------------------------------------
#define MAX_SAMPLERATE			44100
#define MAX_CHANNELS			2
#define MAX_DATA_ELEMS			(MAX_CHANNELS * MAX_SAMPLERATE)
#define MAX_WINDOWING_ELEMS		MAX_SAMPLERATE

#define SILENCE_VALUE			0.0f
#define NORM_VALUE				((float)0x8000)


//------------------------------------------------------------------------------
// FFT_AUDIO LOCAL MACROS
//------------------------------------------------------------------------------
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


//------------------------------------------------------------------------------
// FFT_AUDIO LOCAL STRUCT DEFINITIONS
//------------------------------------------------------------------------------
typedef struct {
	fftwf_plan plan;							// FFTW float FFT plan
	SNDFILE * file;								// Pointer to the audio file
	float data[MAX_DATA_ELEMS];					// Float audio values
	float windowing_data[MAX_DATA_ELEMS];		// Float windowing values
	fftwf_complex fft_in[MAX_WINDOWING_ELEMS];	// Complex audio values
	fftwf_complex fft_out[MAX_WINDOWING_ELEMS];	// Complex FFT values

	size_t samplerate;							// Samplerate of the audio file
	size_t channels;							// Num. of channels of the audio

	size_t frames_elements;						// Num. of elems in a frame
	enum fft_audio_windowing windowing;			// Windowing method
	fft_audio_stats stats;						// Statistics of current frame
} fft_audio;


//------------------------------------------------------------------------------
// FFT_AUDIO LOCAL DATA
//------------------------------------------------------------------------------
static char * windowing_names[] = {
	"Rectangular",
	"Welch",
	"Triangular",
	"Barlett",
	"Hanning",
	"Hamming",
	"Blackman"
};

static fft_audio audio;


//------------------------------------------------------------------------------
//
// This function calculates and stores the rectangular windowing data.
// Each element is calculated as follows:
// w[i] = 1
//
//------------------------------------------------------------------------------
static void fft_audio_fill_windowing_data_rectangular()
{
	size_t i;
	size_t N = audio.frames_elements;

	for (i = 0; i < N; ++i) {
		audio.windowing_data[i] = 1.0f;
	}
}


//------------------------------------------------------------------------------
//
// This function calculates and stores the "Welch" windowing data.
// Each element is calculated as follows:
// w[i] = 1 - [(i - 0.5 * (N - 1)) / (0.5 * (N + 1))] ^ 2
//
//------------------------------------------------------------------------------
static void fft_audio_fill_windowing_data_welch()
{
	size_t i;
	const size_t N = audio.frames_elements;
	float val;

	for (i = 0; i < N; ++i) {
		val = (i - 0.5f * (N - 1)) / (0.5f * N + 1);
		audio.windowing_data[i] = 1.0f - val * val;
	}
}


//------------------------------------------------------------------------------
//
// This function calculates and stores the "Triangular" windowing data.
// Each element is calculated as follows:
// w[i] = (2 / N) * [(N / 2) - |i - (N / 2)|]
//
//------------------------------------------------------------------------------
static void fft_audio_fill_windowing_data_triangular()
{
	size_t i;
	const size_t N = audio.frames_elements;
	const float val = N / 2.0f;

	for (i = 0; i < N; ++i) {
		audio.windowing_data[i] = (val - fabsf(i - val)) / val;
	}
}


//------------------------------------------------------------------------------
//
// This function calculates and stores the "Barlett" windowing data.
// Each element is calculated as follows:
// w[i] = [2 / (N - 1)] * [(N - 1) / 2 - |i - (N - 1) / 2|]
//
//------------------------------------------------------------------------------
static void fft_audio_fill_windowing_data_barlett()
{
	size_t i;
	const size_t N = audio.frames_elements;
	const float val = (N - 1) / 2.0f;

	for (i = 0; i < N; ++i) {
		audio.windowing_data[i] = (val - fabsf(i - val)) / val;
	}
}


//------------------------------------------------------------------------------
//
// This function calculates and stores the "Hanning" windowing data.
// Each element is calculated as follows:
// w[i] = 0.5 * [1 - cos( (2 * pi * i) / (N - 1) )]
//
//------------------------------------------------------------------------------
static void fft_audio_fill_windowing_data_hanning()
{
	size_t i;
	const size_t N = audio.frames_elements;
	float val;

	for (i = 0; i < N; ++i) {
		val = cosf(2 * M_PI * i / (N - 1));
		audio.windowing_data[i] = 0.5f * (1.0f - val);
	}
}


//------------------------------------------------------------------------------
//
// This function calculates and stores the "Hamming" windowing data.
// Each element is calculated as follows:
// a = 0.53836
// b = 0.46164
// w[i] = a - b * cos( (2 * pi * i) / (N - 1) )
//
//------------------------------------------------------------------------------
static void fft_audio_fill_windowing_data_hamming()
{
	size_t i;
	const size_t N = audio.frames_elements;
	float val;

	for (i = 0; i < N; ++i) {
		val = cosf(2 * M_PI * i / (N - 1));
		audio.windowing_data[i] = 0.53836f - 0.46164f * val;
	}
}


//------------------------------------------------------------------------------
//
// This function calculates and stores the "Blackman" windowing data.
// Each element is calculated as follows:
// a = 0.42
// b = 0.5
// c = 0.08
// w[i] = a - b * cos( (2 * pi * i) / (N - 1) + c * cos( (4 * pi * i) / (N - 1)
//
//------------------------------------------------------------------------------
static void fft_audio_fill_windowing_data_blackman()
{
	size_t i;
	const size_t N = audio.frames_elements;
	float val_one;
	float val_two;

	for (i = 0; i < N; ++i) {
		val_one = cosf(2 * M_PI * i / (N - 1));
		val_two = cosf(4 * M_PI * i / (N - 1));
		audio.windowing_data[i] = 0.42f - 0.5f * val_one + 0.08f * val_two;
	}
}


//------------------------------------------------------------------------------
//
// This function is a help function that allows to call the proper windowing
// function that calculates and stores the windowing data.
//
//------------------------------------------------------------------------------
static void fft_audio_fill_windowing_data(const enum fft_audio_windowing windowing)
{

	switch (windowing) {
		case fft_audio_rectangular:
			fft_audio_fill_windowing_data_rectangular();
			break;
		case fft_audio_welch:
			fft_audio_fill_windowing_data_welch();
			break;
		case fft_audio_triangular:
			fft_audio_fill_windowing_data_triangular();
			break;
		case fft_audio_barlett:
			fft_audio_fill_windowing_data_barlett();
			break;
		case fft_audio_hanning:
			fft_audio_fill_windowing_data_hanning();
			break;
		case fft_audio_hamming:
			fft_audio_fill_windowing_data_hamming();
			break;
		case fft_audio_blackman:
			fft_audio_fill_windowing_data_blackman();
			break;
		default:
			fft_audio_fill_windowing_data_rectangular();
			break;
	}
}


//------------------------------------------------------------------------------
//
// This function is a help function that applys the windowing to the current
// frame.
//
//------------------------------------------------------------------------------
static void fft_audio_apply_window()
{
	size_t i;

	for (i = 0; i < audio.frames_elements; ++i) {
		audio.fft_in[i][0] *= audio.windowing_data[i];
	}
}


//------------------------------------------------------------------------------
//
// This function is a help function that allows to load the audio data not  yet
// read. It also performs the numeric normalization of the audio signal needed
// for the FFT execution.
//
//------------------------------------------------------------------------------
static int fft_audio_read_data()
{
	size_t readcount;
	size_t i;
	size_t j;
	size_t index;
	float aux;

	assert(audio.frames_elements * audio.channels <= MAX_DATA_ELEMS);

	readcount = sf_read_float(audio.file, audio.data, audio.frames_elements * audio.channels);
	if (readcount == 0) {
		return FFT_AUDIO_EOF;
	}

	for (i = 0; i < readcount / audio.channels; ++i) {
		aux = 0.0f;
		for (j = 0; j < audio.channels; ++j) {
			index = i * audio.channels + j;
			if (index < readcount) {
				aux += audio.data[index];
			}
		}
		audio.fft_in[i][0] = aux;
	}

	// Set all remaining values to 0 if needed
	for (i = readcount; i < audio.frames_elements; ++i) {
		audio.fft_in[i][0] = 0.0f;
	}

	return FFT_AUDIO_SUCCESS;
}


//------------------------------------------------------------------------------
//
// This function initialize alla data required to perform the FFT and to extract
// statistics from an audio file.
// It opens the file provided, initializes the audio data and the data needed to
// perform the FFT.
//
//------------------------------------------------------------------------------
int fft_audio_init(const char filename[],
				   const size_t frames_elements,
				   const enum fft_audio_windowing windowing)
{
	size_t i;
	SF_INFO info;

	assert(filename != NULL);
	assert(frames_elements <= MAX_WINDOWING_ELEMS);
	assert(fft_audio_rectangular <= windowing);
	assert(windowing <= fft_audio_blackman);

	memset(&info, 0, sizeof(info));
	audio.file = sf_open(filename, SFM_READ, &info);

	if (audio.file == NULL) {
		return FFT_AUDIO_ERROR_FILE;
	}

	if (info.samplerate > MAX_SAMPLERATE) {
		return FFT_AUDIO_ERROR_SAMPLERATE;
	}

	if (info.channels > MAX_CHANNELS) {
		return FFT_AUDIO_ERROR_CHANNELS;
	}

	audio.samplerate		= info.samplerate;
	audio.channels			= info.channels;
	audio.frames_elements	= frames_elements;
	audio.windowing 		= windowing;

	for (i = 0 ; i < MAX_DATA_ELEMS; ++i) {
		audio.data[i] = SILENCE_VALUE;
	}

	for (i = 0; i < MAX_WINDOWING_ELEMS; ++i) {
		audio.fft_in[i][0] = SILENCE_VALUE;
		audio.fft_in[i][1] = 0.0f;
		audio.fft_out[i][0] = 0.0f;
		audio.fft_out[i][1] = 0.0f;
	}

	audio.plan = fftwf_plan_dft_1d(frames_elements,
								   audio.fft_in,
								   audio.fft_out,
								   FFTW_FORWARD,
								   FFTW_ESTIMATE);

	return FFT_AUDIO_SUCCESS;
}


//------------------------------------------------------------------------------
//
// This function returns the samplerate of the provided audio file.
//
//------------------------------------------------------------------------------
size_t fft_audio_get_samplerate()
{
	return audio.samplerate;
}


//------------------------------------------------------------------------------
//
// This function returns the number of channels of the provided audio file.
//
//------------------------------------------------------------------------------
size_t fft_audio_get_channels()
{
	return audio.channels;
}


//------------------------------------------------------------------------------
//
// This function returns the string name of the provided windowing.
//
//------------------------------------------------------------------------------
char * fft_audio_get_windowing_name(enum fft_audio_windowing windowing)
{
	assert(fft_audio_rectangular <= windowing);
	assert(windowing <= fft_audio_blackman);

	return windowing_names[windowing];
}


//------------------------------------------------------------------------------
//
// This function loads the next frame values from the provided audio file.
//
//------------------------------------------------------------------------------
int fft_audio_load_next_frame()
{
	int ret;

	ret = fft_audio_read_data();
	if (ret == FFT_AUDIO_EOF) {
		return FFT_AUDIO_EOF;
	}

	return FFT_AUDIO_SUCCESS;
}


//------------------------------------------------------------------------------
//
// This function computes the FFT of the current frame values applying the 
// windowing method provided.
//
//------------------------------------------------------------------------------
void fft_audio_compute_fft(enum fft_audio_windowing windowing)
{
	assert(fft_audio_rectangular <= windowing);
	assert(windowing <= fft_audio_blackman);

	if (audio.windowing != windowing) {
		audio.windowing = windowing;
		fft_audio_fill_windowing_data(windowing);
	}
	fft_audio_apply_window();
	fftwf_execute(audio.plan);
}


//------------------------------------------------------------------------------
//
// This function fill the provided "buffer" with current frame audio values.
//
//------------------------------------------------------------------------------
void fft_audio_fill_buffer_data(float * buffer)
{
	size_t i;

	assert(buffer != NULL);

	for (i = 0; i < audio.frames_elements * audio.channels; ++i) {
		buffer[i] = audio.data[i];
	}
}


//------------------------------------------------------------------------------
//
// This function returns the statistics of the current FFT audio frame.
//
//------------------------------------------------------------------------------
fft_audio_stats fft_audio_get_stats()
{
	return fft_audio_get_stats_samples(1, audio.frames_elements);
}


//------------------------------------------------------------------------------
//
// This function returns the statistics of the FFT audio in the range of samples
// [from, to]. It calculates the minimum, average and maximum magnitude of each
// sample of the FFT in the current frame.
//
//------------------------------------------------------------------------------
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

	assert(from <= audio.frames_elements);
	assert(to <= audio.frames_elements);

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


//------------------------------------------------------------------------------
//
// This function frees all data and data structures used.
//
//------------------------------------------------------------------------------
void fft_audio_free()
{
	if (audio.file != NULL) {
		sf_close(audio.file);
	}

	if (audio.plan != NULL){
		fftwf_destroy_plan(audio.plan);
	}
}
