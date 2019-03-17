#ifndef CONSTANTS_H
#define CONSTANTS_H

#define FALSE						0
#define TRUE						1

#define FILENAME_SIZE				128
#define BUBBLES_TEXT_SIZE			64
#define TIMER_TEXT_SIZE				16
#define GAIN_TEXT_SIZE				16

#define DISPLAY_W					960
#define DISPLAY_H					480
#define DISPLAY_AUDIO_X				0
#define DISPLAY_AUDIO_Y				64
#define DISPLAY_AUDIO_W				DISPLAY_W
#define DISPLAY_AUDIO_H				(DISPLAY_H - (DISPLAY_AUDIO_Y * 2))
#define FRAMERATE					50

#define TASK_INPUT_PERIOD			30
#define TASK_INPUT_DEADLINE			30
#define TASK_INPUT_PRIORITY			30
#define TASK_INPUT_EVENT_TIME		(20 / 1000.f)

#define TASK_DISPLAY_PERIOD			30
#define TASK_DISPLAY_DEADLINE		30
#define TASK_DISPLAY_PRIORITY		30

#define TASK_FFT_PERIOD				20
#define TASK_FFT_DEADLINE			20
#define TASK_FFT_PRIORITY			90

#define TASK_BUBBLE_PERIOD			20
#define TASK_BUBBLE_DEADLINE		20
#define TASK_BUBBLE_PRIORITY		60


#define BUBBLES_INFO_TEXT_ALIGN		ALLEGRO_ALIGN_LEFT
#define BUBBLES_INFO_X 				10
#define BUBBLES_INFO_Y 				0

#define WINDOWING_INFO_TEXT_ALIGN	ALLEGRO_ALIGN_LEFT
#define WINDOWING_INFO_X 			10
#define WINDOWING_INFO_Y 			20

#define TIMER_INFO_TEXT_ALIGN		ALLEGRO_ALIGN_RIGHT
#define TIMER_INFO_X				DISPLAY_W - 10
#define TIMER_INFO_Y				0

#define GAIN_INFO_TEXT_ALIGN		ALLEGRO_ALIGN_RIGHT
#define GAIN_INFO_X					DISPLAY_W - 10
#define GAIN_INFO_Y					20

#define USER_INFO_TEXT				"[UP: Bigger]  [DOWN: Smaller]  " \
									"[LEFT: Less]  [RIGHT: More]  " \
									"[PLUS: Vol. Up]  [MINUS: Vol. Down]"
#define USER_INFO_TEXT_ALIGN 		ALLEGRO_ALIGN_CENTER
#define USER_INFO_X 				DISPLAY_W / 2
#define USER_INFO_Y 				DISPLAY_H - 32

#define FRAGMENT_COUNT				8
#define FRAGMENT_SAMPLES			882 // Must be a diveder of both FRAGMENT_SAMPLERATE and 1000/FRAMERATE
#define FRAGMENT_DEPTH				ALLEGRO_AUDIO_DEPTH_FLOAT32

#define BUBBLE_TASKS_MIN 			8
#define BUBBLE_TASKS_BASE 			24
#define BUBBLE_TASKS_MAX 			32
#define BUBBLE_FILTER 				0.5

#define BUBBLE_X_OFFSCREEN			-100.0f
#define BUBBLE_Y_OFFSCREEN			-100.0f
#define BUBBLE_THICKNESS			2
#define BUBBLE_RADIUS 				8
#define BUBBLE_ALPHA_FILLED			200
#define BUBBLE_ALPHA_STROKE			255

#define BUBBLE_SCALE_MIN			0.5f
#define BUBBLE_SCALE_BASE			1.0f
#define BUBBLE_SCALE_MAX			2.0f
#define BUBBLE_SCALE_STEP			0.1f

#define GAIN_MIN					0
#define GAIN_BASE					50
#define GAIN_MAX					100
#define GAIN_STEP					1


#define EXIT_NO_FILENAME			128
#define EXIT_OPEN_FILE				129
#define EXIT_ALLEGRO_ERROR			130
#define EXIT_PTASK_CREATE			131
#define EXIT_PTASK_JOIN				132
#define EXIT_BTRAILS_ERROR			133
#define EXIT_FFT_AUDIO_ERROR		134


#define MAX_COLORS 32
const unsigned char colors[MAX_COLORS][3] = {
	//Gray
	{ 93, 109, 126},
	{ 86, 101, 115},
	{ 52,  73,  94},
	{ 46,  64,  83},
	// Blue
	{ 36, 113, 163},
	{ 41, 128, 185},
	{ 46, 134, 193},
	{ 52, 152, 219},
	// Teal
	{ 26, 188, 156},
	{ 23, 165, 137},
	{ 22, 160, 133},
	{ 19, 141, 117},
	// Green
	{ 34, 153,  84},
	{ 39, 174,  96},
	{ 40, 180,  99},
	{ 46, 204, 113},
	// Yellow
	{241, 196,  15},
	{212, 172,  13},
	{243, 156,  18},
	{214, 137,  16},
	// Orange
	{230, 126,  34},
	{202, 111,  30},
	{211,  84,   0},
	{186,  74,   0},
	// Red
	{169,  50,  38},
	{192,  57,  43},
	{203,  67,  53},
	{231,  76,  60},
	// Purple
	{136,  78, 160},
	{155,  89, 182},
	{125,  60, 152},
	{142,  68, 173},
};

#endif
