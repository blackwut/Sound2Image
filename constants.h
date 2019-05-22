#ifndef CONSTANTS_H
#define CONSTANTS_H

//------------------------------------------------------------------------------
// TRUE / FALSE DEFINITIONS
//------------------------------------------------------------------------------
#define FALSE		0
#define TRUE		1


//------------------------------------------------------------------------------
// FONT
//------------------------------------------------------------------------------
#define FONT_NAME			"font/modum.ttf"	// path of the font
#define FONT_SIZE_BIG		18					// font size big
#define FONT_SIZE_SMALL		12					// font size small


//------------------------------------------------------------------------------
// WINDOW / DISPLAY SETTINGS
//------------------------------------------------------------------------------
#define WINDOW_TITLE		"Sound2Image - Alberto Ottimo"
#define DISPLAY_W			960			// display width size in pixel
#define DISPLAY_H			480			// display height size in pixel
#define DISPLAY_TRAILS_X	0			// starting x point of trails display
#define DISPLAY_TRAILS_Y	64			// starting y point of trails display
#define DISPLAY_TRAILS_W	DISPLAY_W	// trails display width size in pixel
#define DISPLAY_TRAILS_H	(DISPLAY_H - (DISPLAY_TRAILS_Y * 2)) // height size


//------------------------------------------------------------------------------
// PERIODIC TASKS SETTINGS
//------------------------------------------------------------------------------
#define TASK_FFT_PERIOD				20				// task_fft period
#define TASK_FFT_DEADLINE			20				// task_fft deadline
#define TASK_FFT_PRIORITY			90				// task_fft priority

#define TASK_BUBBLE_PERIOD			20				// task_bubble period
#define TASK_BUBBLE_DEADLINE		20				// task_bubble deadline
#define TASK_BUBBLE_PRIORITY		60				// task_bubble priority

#define TASK_INPUT_PERIOD			33				// task_input period
#define TASK_INPUT_DEADLINE			33				// task_input deadline
#define TASK_INPUT_PRIORITY			30				// task_input priority
#define TASK_INPUT_EVENT_TIME		(20 / 1000.f)	// task_input event time

#define TASK_DISPLAY_PERIOD			33				// task_display period
#define TASK_DISPLAY_DEADLINE		33				// task_display deadline
#define TASK_DISPLAY_PRIORITY		30				// task_display priority


//------------------------------------------------------------------------------
// USER INFORMATION SETTINGS
//------------------------------------------------------------------------------
#define BUBBLES_INFO_TEXT_ALIGN		ALLEGRO_ALIGN_LEFT	// bubbles text align.
#define BUBBLES_INFO_X 				10					// bubbles x position
#define BUBBLES_INFO_Y 				0					// bubbles y position

#define WINDOWING_INFO_TEXT_ALIGN	ALLEGRO_ALIGN_LEFT	// windowing text align.
#define WINDOWING_INFO_X 			10					// windowing x position
#define WINDOWING_INFO_Y 			20					// windowing y position

#define TIMER_INFO_TEXT_ALIGN		ALLEGRO_ALIGN_RIGHT	// timer text align.
#define TIMER_INFO_X				DISPLAY_W - 10		// timer x position
#define TIMER_INFO_Y				0					// timer y position

#define GAIN_INFO_TEXT_ALIGN		ALLEGRO_ALIGN_RIGHT	// gain text align.
#define GAIN_INFO_X					DISPLAY_W - 10		// gain x position
#define GAIN_INFO_Y					20					// gain y position

// user commands information text
#define USER_INFO_TEXT			"[UP: Bigger]  [DOWN: Smaller]  " \
								"[LEFT: Less]  [RIGHT: More]  " \
								"[PLUS: Vol. Up]  [MINUS: Vol. Down]  " \
								"[1-7: Windowing Method]"
#define USER_INFO_TEXT_ALIGN	ALLEGRO_ALIGN_CENTER	// user info text align.
#define USER_INFO_X 			DISPLAY_W / 2			// user info x position
#define USER_INFO_Y 			DISPLAY_H - 24			// user info y position


//------------------------------------------------------------------------------
// ALLEGRO STREAM SETTINGS
//------------------------------------------------------------------------------
// number of buffers of the stream
#define STREAM_FRAME_COUNT		8
// number of samples provided to the stream. It must be the number of samples
// calculated in the task_fft period and it must be a divider of the samplerate
#define STREAM_SAMPLES			882
// data type of the stream audio values
#define STREAM_DATA_TYPE		ALLEGRO_AUDIO_DEPTH_FLOAT32


//------------------------------------------------------------------------------
// BUBBLE DISPLAY SETTINGS
//------------------------------------------------------------------------------
#define BUBBLE_THICKNESS		2		// thickness of bubble circle
#define BUBBLE_RADIUS 			8		// radius of a bubble
#define BUBBLE_ALPHA_FILLED		200		// alpha channel of filled circle
#define BUBBLE_ALPHA_STROKE		255		// alpha channel of circle


//------------------------------------------------------------------------------
// USER INPUT MIN, BASE, MAX, STEP VALUES
//------------------------------------------------------------------------------
#define BUBBLE_TASKS_MIN 		8		// minimum amount of bubbles
#define BUBBLE_TASKS_BASE 		24		// starting amount of bubbles
#define BUBBLE_TASKS_MAX 		32		// maximum amount of bubbles
#define BUBBLE_LPASS_PARAM		0.5		// low pass filter parameter

#define BUBBLE_SCALE_MIN		0.5f	// minimum scale factor of bubbles
#define BUBBLE_SCALE_BASE		1.0f	// starting scale factor of bubbles
#define BUBBLE_SCALE_MAX		2.0f	// maximum scale factor of bubbles
#define BUBBLE_SCALE_STEP		0.1f	// incr/decr step of scale factor

#define GAIN_MIN				0		// minimum value of gain volume
#define GAIN_BASE				50		// starting value of gain volume
#define GAIN_MAX				100		// maximum value of gain volume
#define GAIN_STEP				1		// incr/decr step of scale volume


//------------------------------------------------------------------------------
// EXIT CONSTANTS
//------------------------------------------------------------------------------
#define EXIT_NO_FILENAME			128		// filename missing error code
#define EXIT_FFT_AUDIO_ERROR		128		// fft_audio generic error code
#define EXIT_FFT_AUDIO_OPEN_FILE	130		// fft_audio open file error code
#define EXIT_FFT_AUDIO_SAMPLERATE	131		// unsupported samplerate error code
#define EXIT_FFT_AUDIO_CHANNELS		132		// unsupported channels error code
#define EXIT_BTRAILS_ERROR			133		// brails generic error code
#define EXIT_PTASK_ERROR			134		// ptask generic error code
#define EXIT_PTASK_CREATE			135		// ptask create task error code
#define EXIT_PTASK_JOIN				136		// ptask join task error code
#define EXIT_ALLEGRO_ERROR			137		// allegro generic error code


//------------------------------------------------------------------------------
// COLORS
//------------------------------------------------------------------------------
// display background color
const unsigned char BACK_COLOR[3] = {32, 32, 32};

// bubble colors
#define MAX_COLORS 32							// number of encoded COLORS
const unsigned char COLORS[MAX_COLORS][3] = {	// global RGB colors
	//Gray
	{ 93, 109, 126}, { 86, 101, 115}, { 52,  73,  94}, { 46,  64,  83},
	// Blue
	{ 36, 113, 163}, { 41, 128, 185}, { 46, 134, 193}, { 52, 152, 219},
	// Teal
	{ 26, 188, 156}, { 23, 165, 137}, { 22, 160, 133}, { 19, 141, 117},
	// Green
	{ 34, 153,  84}, { 39, 174,  96}, { 40, 180,  99}, { 46, 204, 113},
	// Yellow
	{241, 196,  15}, {212, 172,  13}, {243, 156,  18}, {214, 137,  16},
	// Orange
	{230, 126,  34}, {202, 111,  30}, {211,  84,   0}, {186,  74,   0},
	// Red
	{169,  50,  38}, {192,  57,  43}, {203,  67,  53}, {231,  76,  60},
	// Purple
	{136,  78, 160}, {155,  89, 182}, {125,  60, 152}, {142,  68, 173},
};


#endif
