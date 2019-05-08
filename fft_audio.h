//------------------------------------------------------------------------------
//
// FFT_AUDIO
//
// LIBRARY TO SIMPLIFY THE LOADING OF AN AUDIO FILE AND THE COMPUTATION OF THE
// FAST FOURIER TRANSFORM ON IT IN A WINDOWED FASHION.
//
//------------------------------------------------------------------------------
#ifndef FFT_AUDIO_H
#define FFT_AUDIO_H


#include <stdlib.h>


//------------------------------------------------------------------------------
// FFT_AUDIO GLOBAL CONSTANTS
//------------------------------------------------------------------------------
#define FFT_AUDIO_SUCCESS				0
#define FFT_AUDIO_ERROR_FILE			1
#define FFT_AUDIO_ERROR_SAMPLERATE		2
#define FFT_AUDIO_ERROR_CHANNELS		3
#define FFT_AUDIO_EOF					4


//------------------------------------------------------------------------------
// FFT_AUDIO GLOBAL ENUMS DECLARATION
//------------------------------------------------------------------------------
typedef enum {
	fft_audio_rectangular = 0,
	fft_audio_welch,
	fft_audio_triangular,
	fft_audio_barlett,
	fft_audio_hanning,
	fft_audio_hamming,
	fft_audio_blackman
} fft_audio_windowing;


//------------------------------------------------------------------------------
// FFT_AUDIO GLOBAL STRUCTURES DECLARATION
//------------------------------------------------------------------------------
typedef struct {
	size_t from;
	size_t to;
} fft_audio_range;

typedef struct {
	float magMin;
	float magAvg;
	float magMax;
} fft_audio_stats;


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function initializes all data required to perform the FFT and to extract
// statistics from an audio file in a sliding frame fashion.
//
// PARAMETERS
// filename: the path of the audio file
// frame_elements: the number of elements to be computed on each frame
// windowing: the type of windowing to be applied
//
// RETURN
// It returns:
// - FFT_AUDIO_ERROR_FILE if the file does not exists or is not accessible.
// - FFT_AUDIO_ERROR_SAMPLERATE if audio samplerate is greater than 44100
// - FFT_AUDIO_ERROR_CHANNELS if audio channels are more than 2
// - FFT_AUDIO_SUCCESS otherwise
//
//------------------------------------------------------------------------------
int fft_audio_init(const char filename[],
				   const size_t frame_elements,
				   const fft_audio_windowing windowing);

//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns the samplerate of the provided audio file.
//
//
// RETURN
// The samplerate of the provided audio file.
//
//------------------------------------------------------------------------------
size_t fft_audio_get_samplerate();

//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns the number of channels of the provided audio file.
//
// RETURN
// The number of channels of the provided audio file.
//
//------------------------------------------------------------------------------
size_t fft_audio_get_channels();


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns the string name of the provided windowing.
//
// PARAMETERS
// windowing: the windowing enum value whose name you want to know
//
// RETURN
// The string name of the provided windowing
//
//------------------------------------------------------------------------------
char * fft_audio_get_windowing_name(fft_audio_windowing windowing);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function loads the next frame values from the provided audio file.
//
// RETURN
// If there is no data left, it returns FFT_AUDIO_EOF.
// Otherwise it returns FFT_AUDIO_SUCCESS.
//
//------------------------------------------------------------------------------
int fft_audio_load_next_frame();


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function computes the FFT of the current frame values applying the
// windowing method provided.
//
// PARAMETERS
// windowing: the type of window to be applied
//
//------------------------------------------------------------------------------
void fft_audio_compute_fft(fft_audio_windowing windowing);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function fills the provided "buffer" with current frame audio values.
//
// PARAMETERS
// buffer: a float buffer of size (frame_elements * channels)
//
//------------------------------------------------------------------------------
void fft_audio_fill_buffer_data(float * buffer);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns the statistics of the current FFT audio frame values.
//
//
// RETURN
// The statistics of the current FFT audio frame values. The statistics contains
// the minimum, average and maximum magnitude of the FFT.
//
//------------------------------------------------------------------------------
fft_audio_stats fft_audio_get_stats();


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns the statistics of the FFT audio in the range of samples
// [from, to].
//
// PARAMETERS
// range: the range [from, to] in which calculate the statistics
//
// RETURN
// The statistics of the FFT audio in the range of samples [from, to]
//
//------------------------------------------------------------------------------
fft_audio_stats fft_audio_get_stats_samples(const fft_audio_range range);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function frees all data and data structures used.
//
//------------------------------------------------------------------------------
void fft_audio_free();


#endif
