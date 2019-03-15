//------------------------------------------------------------------------------
//
// FFT_AUDIO
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
#define FFT_AUDIO_ERROR_OPEN_FILE		1
#define FFT_AUDIO_EOF					2


//------------------------------------------------------------------------------
// FFT_AUDIO GLOBAL ENUMS DECLARATION
//------------------------------------------------------------------------------
enum fft_audio_window {
	fft_audio_rectangular = 0,
	fft_audio_welch,
	fft_audio_triangular,
	fft_audio_barlett,
	fft_audio_hanning,
	fft_audio_hamming,
	fft_audio_blackman
};


//------------------------------------------------------------------------------
// FFT_AUDIO GLOBAL STRUCTURES DECLARATION
//------------------------------------------------------------------------------
typedef struct fft_audio_stats {
	float magMin;
	float magAvg;
	float magMax;
} fft_audio_stats;


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function initializes all data required to perform the FFT and extract
// some statistics from an audio file.
//
// PARAMETERS
// filename: the path of the audio file
// window_elements: the number of elements to be computed on each window
// windowing: the type of window to be applied
//
// RETURN
// If the file does not exists or it is not accessible, this functions returns
// FFT_AUDIO_ERROR_OPEN_FILE.
// Otherwhise it returns FFT_AUDIO_SUCCESS.
//
//------------------------------------------------------------------------------
int fft_audio_init(const char filename[],
				   const size_t window_elements,
				   const enum fft_audio_window windowing);

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
char * fft_audio_get_windowing_name(enum fft_audio_window windowing);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function loads the next window values from the provided audio file.
//
// RETURN
// If there is no data left, it returns FFT_AUDIO_EOF.
// Otherwise it returns FFT_AUDIO_SUCCESS.
//
//------------------------------------------------------------------------------
int fft_audio_load_next_window();


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function computes the FFT of the current window values applying the 
// windowing method provided.
//
// PARAMETERS
// windowing: the type of window to be applied
//
//------------------------------------------------------------------------------
void fft_audio_compute_fft(enum fft_audio_window windowing);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function fill the provided "buffer" with current window audio values.
//
// PARAMETERS
// buffer: a float buffer of size (window_elements * channels)
//
//------------------------------------------------------------------------------
void fft_audio_fill_buffer_data(float * buffer);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns the statistics of the current FFT audio window.
//
//
// RETURN
// The statistics of the current FFT audio window. The statistics contains the
// minimum, average and maximum magnitude of the FFT.
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
// from: the start sample position of the range
// to: the end sample position of the range
//
// RETURN
// The statistics of the FFT audio in the range of samples [from, to]
//
//------------------------------------------------------------------------------
fft_audio_stats fft_audio_get_stats_samples(const size_t from,
											const size_t to);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function frees all data and data structures used for the computation.
//
//------------------------------------------------------------------------------
void fft_audio_free();


#endif
