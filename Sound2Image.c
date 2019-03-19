#include <stdio.h>
#include <stdlib.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <pthread.h>
#include <math.h>
#include "constants.h"
#include "fft_audio.h"
#include "btrails.h"
#include "ptask.h"
#include "time_utils.h"


//------------------------------------------------------------------------------
// SOUND2IMAGE GLOBAL MACROS
//------------------------------------------------------------------------------
#define MUTEX_LOCK(lock)	pthread_mutex_lock(&lock)	// Shorthand for lock
#define MUTEX_UNLOCK(lock)	pthread_mutex_unlock(&lock)	// Shorthand for unlock

// Shorthand for protect a small critical section
#define MUTEX_EXP(expr, lock) \
	MUTEX_LOCK(lock); \
	expr; \
	MUTEX_UNLOCK(lock)


//------------------------------------------------------------------------------
// SOUND2IMAGE FUNCTION PROTOTYPES
//------------------------------------------------------------------------------

// Check functions
void fft_audio_check(int ret,
					 const char * description);
void btrails_check(int ret,
				   const char * description);
void ptask_check(int ret,
				 const char * description);
void allegro_check(bool test,
				   const char * description);

// Allegro help functions
void allegro_init();
ALLEGRO_CHANNEL_CONF allegro_channel_conf_with(size_t channels);
void allegro_stream_set_gain(size_t val);
int allegro_stream_fill_frame();
void allegro_blender_mode_standard();
void allegro_blender_mode_alpha();
void allegro_free();

// Bubble help functions
float bubble_spacing_with(size_t n);
fft_audio_range bubble_samples_range(const size_t id,
									 const size_t n);
float bubble_calculate_val(const size_t id,
						   const float val_old,
						   const fft_audio_range range);

// Draw help functions
void draw_trail(const size_t id,
				const float spacing,
				const float scale);
void draw_trails();
void draw_bubbles_info();
void draw_windowing_info();
void draw_time_info();
void draw_gain_info();
void draw_user_info();

// Task handlers
void * task_fft(void * arg);
void * task_bubble(void * arg);
void * task_display(void * arg);
void * task_input(void * arg);


//------------------------------------------------------------------------------
// SOUND2IMAGE GLOBAL DATA
//------------------------------------------------------------------------------
ALLEGRO_DISPLAY * display;			// the main area of drawing
ALLEGRO_EVENT_QUEUE * input_queue;	// queue managing user input events
ALLEGRO_COLOR font_color;			// the color of drawn text
ALLEGRO_FONT * font_big;			// font used to draw text with big size
ALLEGRO_FONT * font_small;			// font used to draw text with small size

size_t samplerate;					// samplerate of audio file.
int done;							// if TRUE the program stops
size_t active_tasks;				// num. of tasks active
size_t gain;						// volume of audio listening
fft_audio_windowing windowing;		// windowing method of FFT
size_t time_ms;						// elapsed time of audio listening
float bubble_scale;					// scale factor of displayed bubbles
ALLEGRO_AUDIO_STREAM * stream;		// audio stream object

// Mutexes
pthread_mutex_t mux_done;			// mutex associated to variable done
pthread_mutex_t mux_active_tasks;	// mutex associated to variable active_tasks
pthread_mutex_t mux_gain;			// mutex associated to variable gain
pthread_mutex_t mux_windowing;		// mutex associated to variable windowing
pthread_mutex_t mux_time_ms;		// mutex associated to variable time_ms
pthread_mutex_t mux_bubble_scale;	// mutex associated to variable bubble_scale

//------------------------------------------------------------------------------
// Mutex, Condition variables and counter to implement precedence relationship
// between task_fft and all task_bubble periodic tasks.
//------------------------------------------------------------------------------
pthread_cond_t cond_fft_producer;	// cond. var. associated to task_fft
pthread_cond_t cond_fft_consumers;	// cond. var. associated to all task_bubble
pthread_mutex_t mux_fft;			// mutex associated to the prev. cond. var.
size_t fft_counter;					// variable to make synchronization


//------------------------------------------------------------------------------
// SOUND2IMAGE FUNCTION DEFINITIONS
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function checks if a fft_audio function returned with an error. If an
// error occurred, it prints an error message and exits with the corresponding
// error value.
//
// PARAMETERS
// ret: the return value of the fft_audio function
// description: user error info description
//
//------------------------------------------------------------------------------
void fft_audio_check(int ret,
					 const char * description)
{
	if (ret == FFT_AUDIO_SUCCESS) return;
	fprintf(stderr, "FFT_AUDIO ERROR - %s\n", description);

	if (ret == FFT_AUDIO_ERROR_FILE) exit(EXIT_FFT_AUDIO_OPEN_FILE);
	if (ret == FFT_AUDIO_ERROR_SAMPLERATE) exit(EXIT_FFT_AUDIO_SAMPLERATE);
	if (ret == FFT_AUDIO_ERROR_CHANNELS) exit(EXIT_FFT_AUDIO_CHANNELS);
	exit(EXIT_FFT_AUDIO_ERROR);
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function checks if a btrail function returned with an error. If an
// error occurred, it prints an error message and exits with the corresponding
// error value.
//
// PARAMETERS
// ret: the return value of the btrail function
// description: user error info description
//
//------------------------------------------------------------------------------
void btrails_check(int ret,
				   const char * description)
{
	if (ret == BTRAILS_SUCCESS) return;
	fprintf(stderr, "BRAILS ERROR - %s\n", description);
	exit(BTRAILS_ERROR);
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function checks if a ptask function returned with an error. If an error
// occurred, it prints an error message and exits with the corresponding error
// value.
//
// PARAMETERS
// ret: the return value of the ptask function
// description: user error info description
//
//------------------------------------------------------------------------------
void ptask_check(int ret,
				 const char * description)
{
	if (ret == PTASK_SUCCESS) return;
	fprintf(stderr, "PTASK ERROR - %s\n", description);

	if (ret == PTASK_ERROR_CREATE) exit(EXIT_PTASK_CREATE);
	if (ret == PTASK_ERROR_JOIN) exit(EXIT_PTASK_JOIN);
	exit(EXIT_PTASK_ERROR);
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function checks if a Allegro function returned with an error. If an
// error occurred, it prints an error message and exits with the corresponding
// error value.
//
// PARAMETERS
// ret: the return value of the Allegro function
// description: user error info description
//
//------------------------------------------------------------------------------
void allegro_check(bool ret,
				   const char * description)
{
	if (ret) return;
	fprintf(stderr, "ALLEGRO ERROR - %s\n", description);
	exit(EXIT_ALLEGRO_ERROR);
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function initializes and configure all Allegro addon and all global
// variables used.
//
//------------------------------------------------------------------------------
void allegro_init()
{
	allegro_check(al_init(), "al_init()");

	// Display
	al_set_new_window_title(WINDOW_TITLE);
	al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
	al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
	al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR |
							ALLEGRO_MAG_LINEAR |
							ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
	display = al_create_display(DISPLAY_W, DISPLAY_H);
	allegro_check(display, "al_create_display()");

	// User input
	input_queue = al_create_event_queue();
	allegro_check(input_queue, "al_create_event_queue()");
	allegro_check(al_install_keyboard(), "al_install_keyboard()");
	al_register_event_source(input_queue, al_get_display_event_source(display));
	al_register_event_source(input_queue, al_get_keyboard_event_source());

	// Audio
	al_init_acodec_addon();
	allegro_check(al_install_audio(), "al_install_audio()");
	allegro_check(al_reserve_samples(0), "al_reserve_samples()");

	// Primitives
	allegro_check(al_init_primitives_addon(), "al_init_primitives_addon()");

	// Font
	allegro_check(al_init_font_addon(), "al_init_font_addon()");
	allegro_check(al_init_ttf_addon(), "al_init_ttf_addon()");
	font_color = al_map_rgba(192,  57,  43, 255);
	font_big = al_load_ttf_font(FONT_NAME, FONT_SIZE_BIG, 0);
	font_small = al_load_ttf_font(FONT_NAME, FONT_SIZE_SMALL, 0);
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function return the ALLEGRO_CHANNEL_CONF value corresponding to the
// number of channels provided.
//
// PARAMETERS
// channels: the number of channel
//
// RETURN
// The ALLEGRO_CHANNEL_CONF corresponding to the number of channels
//
//------------------------------------------------------------------------------
ALLEGRO_CHANNEL_CONF allegro_channel_conf_with(size_t channels)
{
	return (channels == 2 ? ALLEGRO_CHANNEL_CONF_2 : ALLEGRO_CHANNEL_CONF_1);
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function set the given gain to the stream audio.
//
// PARAMETERS
// val: the value of the new gain to set
//
//------------------------------------------------------------------------------
void allegro_stream_set_gain(size_t val)
{
	allegro_check(al_set_audio_stream_gain(stream, val / 100.0f),
				  "al_set_audio_stream_gain()");
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function tries to copy the current frame audio values into a buffer
// provided by the streaming object.
//
// RETURN
// It returns TRUE if the frame is copied into the buffer.
// Otherwise it return FALSE.
//
//------------------------------------------------------------------------------
int allegro_stream_fill_frame()
{
	float * buffer = NULL;

	buffer = al_get_audio_stream_fragment(stream);
	if (buffer != NULL) {
		fft_audio_fill_buffer_data(buffer);
		allegro_check(al_set_audio_stream_fragment(stream, buffer),
					  "al_set_audio_stream_fragment()");
		return TRUE;
	}
	return FALSE;
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function set the blender mode to:
// ALLEGRO_ADD - ALLEGRO_ONE - ALLEGRO_INVERSE_ALPHA
// That means that each pixel color is calculated as follows:
// r = d.r * (1 - s.a) + s.r * 1
// g = d.g * (1 - s.a) + s.g * 1
// b = d.b * (1 - s.a) + s.b * 1
// a = d.a * (1 - s.a) + s.a * 1
//
// Where "d" is the destrination bitmap/display pixel color and "s" is the pixel
// color to be drawn.
//
//------------------------------------------------------------------------------
void allegro_blender_mode_standard()
{
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function set the blender mode to:
// ALLEGRO_ADD - ALLEGRO_ALPHA - ALLEGRO_INVERSE_ALPHA
// That means that each pixel color is calculated as follows:
// r = d.r * (1 - s.a) + s.r * s.a
// g = d.g * (1 - s.a) + s.g * s.a
// b = d.b * (1 - s.a) + s.b * s.a
// a = d.a * (1 - s.a) + s.a * s.a
//
// Where "d" is the destrination bitmap/display pixel color and "s" is the pixel
// color to be drawn.
//
//------------------------------------------------------------------------------
void allegro_blender_mode_alpha()
{
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function uninstall, shutdown and destroy all allocated Allegro
// resources used.
//
//------------------------------------------------------------------------------
void allegro_free()
{
	al_uninstall_keyboard();
	al_uninstall_audio();
	al_shutdown_primitives_addon();
	al_destroy_font(font_big);
	al_destroy_font(font_small);
	al_shutdown_ttf_addon();
	al_shutdown_font_addon();
	al_destroy_display(display);
	al_destroy_event_queue(input_queue);
	al_uninstall_system();
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function initializes all variables used by the program including:
// fft_audio, btrail, ptask and Allegro libraries. It also open the audio file
// located in "filename".
//
// PARAMETERS
// filename: the relative path of the audio file
//
//------------------------------------------------------------------------------
void sound2image_init_variables(char * filename)
{
	size_t channels;		// number of channels of the audio file

	done = FALSE;
	fft_counter = 0;
	active_tasks = BUBBLE_TASKS_BASE;
	bubble_scale = BUBBLE_SCALE_BASE;
	time_ms = 0;
	gain = GAIN_BASE;
	windowing = fft_audio_rectangular;

	pthread_mutex_init(&mux_done, NULL);
	pthread_mutex_init(&mux_time_ms, NULL);
	pthread_mutex_init(&mux_bubble_scale, NULL);
	pthread_mutex_init(&mux_active_tasks, NULL);

	pthread_cond_init(&cond_fft_producer, NULL);
	pthread_cond_init(&cond_fft_consumers, NULL);
	pthread_mutex_init(&mux_fft, NULL);
	pthread_mutex_init(&mux_gain, NULL);
	pthread_mutex_init(&mux_windowing, NULL);

	fft_audio_check(fft_audio_init(filename,
								   STREAM_SAMPLES,
								   fft_audio_hamming),
					"File does not exits or it is not compatible");
	samplerate = fft_audio_get_samplerate();
	channels = fft_audio_get_channels();

	allegro_init();
	al_set_target_bitmap(NULL);

	stream = al_create_audio_stream(STREAM_FRAME_COUNT,
									STREAM_SAMPLES,
									samplerate,
									STREAM_DATA_TYPE,
									allegro_channel_conf_with(channels));
	al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_ONCE);
	allegro_stream_set_gain(gain);
	allegro_check(al_attach_audio_stream_to_mixer(stream,
												  al_get_default_mixer()),
				  "al_attach_audio_stream_to_mixer()");

	btrails_check(btrails_init(), "Cannot create Bubble Trails");
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function create all periodic tasks needed for the execution.
//
//------------------------------------------------------------------------------
void sound2image_create_tasks()
{
	size_t i;		// The index of periodic task created

	for (i = 0; i < BUBBLE_TASKS_MAX; ++i) {
		ptask_check(ptask_create(task_bubble,
								 i,
								 TASK_BUBBLE_PERIOD,
								 TASK_BUBBLE_DEADLINE,
								 TASK_BUBBLE_PRIORITY),
					"task_bubble");
	}

	ptask_check(ptask_create(task_fft,
							 i++,
							 TASK_FFT_PERIOD,
							 TASK_FFT_DEADLINE,
							 TASK_FFT_PRIORITY),
				"task_fft");
	ptask_check(ptask_create(task_input,
							 i++,
							 TASK_INPUT_PERIOD,
							 TASK_INPUT_DEADLINE,
							 TASK_INPUT_PRIORITY),
				"task_input");
	ptask_check(ptask_create(task_display,
							 i++,
							 TASK_DISPLAY_PERIOD,
							 TASK_DISPLAY_DEADLINE,
							 TASK_DISPLAY_PRIORITY),
				"task_display");
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function awaits the termination of all periodic tasks.
//
//------------------------------------------------------------------------------
void sound2image_waits_tasks()
{
	ptask_check(ptask_wait_tasks(), "ptask_wait_tasks()");
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function relase all variable memory created at runtime.
//
//------------------------------------------------------------------------------
void sound2image_free()
{
	pthread_mutex_destroy(&mux_active_tasks);
	pthread_mutex_destroy(&mux_bubble_scale);
	pthread_mutex_destroy(&mux_time_ms);
	pthread_mutex_destroy(&mux_done);
	pthread_cond_destroy(&cond_fft_producer);
	pthread_cond_destroy(&cond_fft_consumers);
	pthread_mutex_destroy(&mux_fft);
	pthread_mutex_destroy(&mux_gain);
	pthread_mutex_destroy(&mux_windowing);

	btrails_free();
	fft_audio_free();
	al_drain_audio_stream(stream);
	allegro_free();
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns the spacing between two bubbles.
// This values is calculated based on the portion of the screen reserved to
// display the bubbles.
//
// PARAMETERS
// n: the number of active bubbles
//
// RETURN
// The spacing between two bubbles
//
//------------------------------------------------------------------------------
float bubble_spacing_with(size_t n)
{
	return DISPLAY_W / (float)(n + 1);
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function calculates the range [from, to] of samples for a given bubble.
// Values "from" and "to" are calculated as follows:
// N    = number of samples
// step = log_2( (N / 2 - n) / (n + 1) )
// from = round( 2^( step * (id + 1) ) ) + (id + 1)
// to   = round( 2^( step * (id + 2) ) ) + (id + 2)
//
// PARAMETERS
// id: the bubble id whose range you want to calculate
// n: the total number of active bubbles
//
// RETURN
// The range [from, to] of samples for a given bubble.
// If the buble id is greather or equal to "n", the range returned is [0, 0].
//
//------------------------------------------------------------------------------
fft_audio_range bubble_samples_range(const size_t id,
									 const size_t n)
{
	float step;					// the step exponent to calculate from and to
	fft_audio_range range;		// the range [from, to] to be calculated

	if (id < n) {
		step = log2f(STREAM_SAMPLES / 2 - n) / (n + 1);
		range.from = (size_t)lroundf(powf(2, (id + 1) * step)) + id + 1;
		range.to = (size_t)lround(powf(2, (id + 2) * step)) + id + 2;
	} else {
		range.from = 0;
		range.to = 0;
	}

	return range;
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function is an auxiliary function used to calculate a new value needed
// to calculate the y coordinate of a new bubble.
// This values is calculated based on the statistics of the current played frame
// and it is filtered by a low-pass filter.
//
// PARAMETERS
// id: the bubble id whose value you want to calculate
// val_old: the old value. It is needed to apply the low-pass filter
// range: the range [from, to] of samples assigned to the bubble
//
// RETURN
// It returns 0 if the new value is not finite. Otherwise the new value.
//
//------------------------------------------------------------------------------
float bubble_calculate_val(const size_t id,
						   const float val_old,
						   const fft_audio_range range)
{
	float val;						// the new value
	float valMin;					// the min value of audio frame statistics
	float valMax;					// the max value of audio frame statistics
	fft_audio_stats audio_stats;	// statistics of the audio frame
	fft_audio_stats frame_stats;	// statistics of the audio in the range

	audio_stats = fft_audio_get_stats();
	frame_stats = fft_audio_get_stats_samples(range);

	valMin = log2f(audio_stats.magMin);
	valMax = log2f(audio_stats.magMax);
	val = log2f(frame_stats.magMax);
	val = (val - valMin) / (valMax - valMin);

	if (isfinite(val)) {
		return val_old * BUBBLE_LPASS_PARAM + (1.0f - BUBBLE_LPASS_PARAM) * val;
	}

	return 0.0f;
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function draws the "id" bubble into the current bitmap/display. It has
// the need of the current spacing between bubbles and the scale factor to draw
// the bubble correctly.
//
// PARAMETERS
// id: the bubble id to draw
// spacing: the spacing between two bubbles
// scale: the scale factor by which the bubble is drawn
//
//------------------------------------------------------------------------------
void draw_trail(const size_t id,
				const float spacing,
				const float scale)
{
	size_t i;					// index of the bubble in the trail
	size_t top;					// index of bubble to be displayed
	size_t freq;				// frequency of the bubble trail
	btrail_color bcolor;		// color of the bubble trail
	btrail_point bpoint;		// coordinates of a bubble
	float alpha;				// alpha channel of the color
	ALLEGRO_COLOR color;		// aux color variable

	btrails_lock(id);
	top = btrails_get_top(id);
	freq = btrails_get_freq(id);
	bcolor = btrails_get_color(id);

	// Draw all information of that trail
	allegro_blender_mode_standard();
	color = al_map_rgba(bcolor.red, bcolor.green, bcolor.blue,
						BUBBLE_ALPHA_STROKE);
	al_draw_textf(font_small, color,
				  (id + 1) * spacing,
				  DISPLAY_TRAILS_H + DISPLAY_TRAILS_Y + 20,
				  ALLEGRO_ALIGN_CENTER,
				  "%zu", freq);

	// Draw all bubbles inside that trail
	allegro_blender_mode_alpha();
	for (i = 0; i < MAX_BELEMS; ++i) {
		top = NEXT_BELEM(top);
		bpoint = btrails_get_bubble_pos(id, i);
		alpha = (MAX_BELEMS - i) / (float) MAX_BELEMS;

		color = al_map_rgba(bcolor.red, bcolor.green, bcolor.blue,
							BUBBLE_ALPHA_FILLED * alpha);
		al_draw_filled_circle(bpoint.x, bpoint.y,
							  BUBBLE_RADIUS * scale,
							  color);

		color = al_map_rgba(bcolor.red, bcolor.green, bcolor.blue,
							BUBBLE_ALPHA_STROKE * alpha);
		al_draw_circle(bpoint.x, bpoint.y,
					   BUBBLE_RADIUS * scale,
					   color,
					   BUBBLE_THICKNESS);
	}
	btrails_unlock(id);
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function draws all the trails into the current bitmap/display.
//
//------------------------------------------------------------------------------
void draw_trails()
{
	size_t i;						// index of the bubble trail
	size_t active_tasks_local;		// local value of active tasks
	float spacing;					// spacing between two bubbles
	float scale;					// scale factor by which the bubble is drawn

	MUTEX_EXP(active_tasks_local = active_tasks, mux_active_tasks);
	MUTEX_EXP(scale = bubble_scale, mux_bubble_scale);

	spacing = bubble_spacing_with(active_tasks_local);

	for (i = 0; i < active_tasks_local; ++i) {
		draw_trail(i, spacing, scale);
	}
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function draws the number of bubbles currently displayed.
//
//------------------------------------------------------------------------------
void draw_bubbles_info()
{
	size_t active_tasks_local;		// local value of active tasks

	MUTEX_EXP(active_tasks_local = active_tasks, mux_active_tasks);

	allegro_blender_mode_standard();
	al_draw_textf(font_big, font_color,
				  BUBBLES_INFO_X, BUBBLES_INFO_Y,
				  BUBBLES_INFO_TEXT_ALIGN,
				  "Bubbles: %zu", active_tasks_local);
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function draws the windowing method currently used.
//
//------------------------------------------------------------------------------
void draw_windowing_info()
{
	int windowing_local;		// local value of windowing method

	MUTEX_EXP(windowing_local = windowing, mux_windowing);
	fft_audio_get_windowing_name(windowing_local);

	allegro_blender_mode_standard();
	al_draw_textf(font_big, font_color,
				  WINDOWING_INFO_X, WINDOWING_INFO_Y,
				  WINDOWING_INFO_TEXT_ALIGN,
				  "Windowing: %s",
				  fft_audio_get_windowing_name(windowing_local));
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function draws the current time seek of the played audio file.
//
//------------------------------------------------------------------------------
void draw_time_info()
{
	size_t minutes;
	size_t seconds;
	size_t milliseconds;

	MUTEX_EXP(milliseconds = time_ms, mux_time_ms);

	minutes = TIME_MSEC_TO_SEC(milliseconds) / 60;
	seconds = TIME_MSEC_TO_SEC(milliseconds) % 60;
	milliseconds = milliseconds % TIME_MILLISEC;

	allegro_blender_mode_standard();
	al_draw_textf(font_big, font_color,
				  TIMER_INFO_X, TIMER_INFO_Y,
				  TIMER_INFO_TEXT_ALIGN,
				  "%02zu:%02zu:%03zu", minutes, seconds, milliseconds);
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function draws the gain volume of the played audio file.
//
//------------------------------------------------------------------------------
void draw_gain_info()
{
	size_t gain_local;		// local value of gain

	MUTEX_EXP(gain_local = gain, mux_gain);

	al_draw_textf(font_big, font_color,
				  GAIN_INFO_X, GAIN_INFO_Y,
				  GAIN_INFO_TEXT_ALIGN,
				  "Vol:%3zu", gain_local);
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function draws the user commands information.
//
//------------------------------------------------------------------------------
void draw_user_info()
{
	allegro_blender_mode_standard();
	al_draw_textf(font_big, font_color,
				  USER_INFO_X, USER_INFO_Y,
				  USER_INFO_TEXT_ALIGN,
				  USER_INFO_TEXT);
}


//------------------------------------------------------------------------------
// SOUND2IMAGE PERIODIC TASK DEFINITIONS
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// DESCRIPTION
// This is the handler of the task that load, fills and makes the FFT on the
// streaming audio values.
//
// PARAMETERS
// arg: a hidden structure pointer to manage the periodic task
//
//------------------------------------------------------------------------------
void * task_fft(void * arg)
{
	const size_t id = ptask_id(arg);		// id of this periodic task
	int done_local = FALSE;					// local value of done
	int windowing_local;					// local value of windowing method
	int ret;								// ret value

	// Activate for the first time this periodic task
	ptask_activate(id);

	while (!done_local) {

		// Wait the computation of all task_bubble
		MUTEX_LOCK(mux_fft);
		while (fft_counter > 0) {
			pthread_cond_wait(&cond_fft_producer, &mux_fft);
		}

		// Provide new values to the audio stream
		ret = allegro_stream_fill_frame();
		if (ret == TRUE) {
			// Update the elapsed audio time
			MUTEX_EXP(time_ms += STREAM_SAMPLES * 1000/ samplerate,
					 mux_time_ms);

			// Load the curent windowing method and compute the FFT
			MUTEX_EXP(windowing_local = windowing, mux_windowing);
			fft_audio_compute_fft(windowing_local);

			// Load a new frame for the next period
			ret = fft_audio_load_next_frame();
			if (ret == FFT_AUDIO_EOF) {
				MUTEX_EXP(done = TRUE, mux_done);
			}
		}

		// Awake all task_bubble to make them computing the new frame
		fft_counter = BUBBLE_TASKS_MAX;
		pthread_cond_broadcast(&cond_fft_consumers);
		MUTEX_UNLOCK(mux_fft);

		// Update the current status of done variable
		MUTEX_EXP(done_local = done, mux_done);

		// Check of a deadline miss and wait for the next period
		if (ptask_deadline_miss(id) == PTASK_DEADLINE_MISS) {
			fprintf(stderr, "%zu) deadline missed! woet: %zu ms\n",
					id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This is the handler of the task that compute the bubble assigned to a range
// to be displayed.
//
// PARAMETERS
// arg: a hidden structure pointer to manage the periodic task
//
//------------------------------------------------------------------------------
void * task_bubble(void * arg)
{
	const size_t id = ptask_id(arg);			// id of this periodic task
	const size_t user_id = ptask_user_id(arg);	// user id of this periodic task
	int done_local = FALSE;						// local value of done
	size_t active_tasks_local;					// local value of active tasks
	float bubble_spacing;						// space between two bubbles
	fft_audio_range range;						// range calculated by this task
	float val = 0.0f;							// value to calculate y bpoint
	float x = 0.0f;								// x value of bpoint
	float y = 0.0f;								// y value of bpoint
	size_t color_id;							// id of the color selected

	ptask_activate(id);

	while (!done_local) {

		// Update the local value of active_tasks and calculate the spacing
		MUTEX_EXP(active_tasks_local = active_tasks, mux_active_tasks);
		bubble_spacing = bubble_spacing_with(active_tasks_local);

		// Wait the computation of task_fft
		MUTEX_LOCK(mux_fft);
		while (fft_counter == 0) {
			pthread_cond_wait(&cond_fft_consumers, &mux_fft);
		}

		// if the task is active calculate the bubble values
		if (user_id < active_tasks_local) {
			// calculate the range of samples assigned to the bubble
			range = bubble_samples_range(user_id, active_tasks_local);
			// calculate the new value to compute the y position of the bubble
			val = bubble_calculate_val(user_id, val, range);
			// calculate the color_id
			color_id = MAX_COLORS * (user_id / (float) active_tasks_local);
			// calculate the x position of the bubble using the current spacing
			x = (user_id + 1) * bubble_spacing;
			// calculate the y position of the bubble using a low-pass filter
			y = DISPLAY_TRAILS_Y + DISPLAY_TRAILS_H - val * DISPLAY_TRAILS_H;

		// if the task is not active, make the bubble offscreen
		} else {
			range.from = 0;
			range.to = 0;
			color_id = 0;
			x = BUBBLE_X_OFFSCREEN;
			y = BUBBLE_Y_OFFSCREEN;
		}

		// Awake the task_fft if all task_bubble completed the computation
		fft_counter--;
		if (fft_counter == 0) {
			pthread_cond_signal(&cond_fft_producer);
		}
		MUTEX_UNLOCK(mux_fft);

		// Put a new bubble into the circular buffer
		btrails_lock(user_id);
		btrails_set_color(user_id,
						  COLORS[color_id][0],
						  COLORS[color_id][1],
						  COLORS[color_id][2]);
		btrails_set_freq(user_id, range.to * samplerate / STREAM_SAMPLES);
		btrails_put_bubble_pos(user_id, x, y);
		btrails_unlock(user_id);

		// Update the current status of done variable
		MUTEX_EXP(done_local = done, mux_done);

		// Check of a deadline miss and wait for the next period
		if (ptask_deadline_miss(id) == PTASK_DEADLINE_MISS) {
			fprintf(stderr, "%zu) deadline missed! woet: %zu ms\n",
					id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This is the handler of the task that draws the GUI.
//
// PARAMETERS
// arg: a hidden structure pointer to manage the periodic task
//
//------------------------------------------------------------------------------
void * task_display(void * arg)
{
	const size_t id = ptask_id(arg);					// id of this task
	int done_local = FALSE;								// local value of done
	ALLEGRO_COLOR color = al_map_rgba(32, 32, 32, 255);	// background color

	// Set the target buffer of drawing functions
	al_set_target_backbuffer(display);

	ptask_activate(id);

	while (!done_local) {

		// Clear the display
		al_clear_to_color(color);

		// Draw trails bubble and user information
		draw_trails();
		draw_bubbles_info();
		draw_windowing_info();
		draw_time_info();
		draw_gain_info();
		draw_user_info();

		al_flip_display();

		// Update the current status of done variable
		MUTEX_EXP(done_local = done, mux_done);

		// Check of a deadline miss and wait for the next period
		if (ptask_deadline_miss(id) == PTASK_DEADLINE_MISS) {
			fprintf(stderr, "%zu) deadline missed! woet: %zu ms\n",
					id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This is the handler of the task that read the input of the user and perform
// the operation associated to that input.
//
// PARAMETERS
// arg: a hidden structure pointer to manage the periodic task
//
//------------------------------------------------------------------------------
void * task_input(void * arg)
{
	const size_t id = ptask_id(arg);	// id of this task
	int done_local = FALSE;				// local value of done
	size_t i;							// index of ALLEGRO_KEY_# "for loop"
	ALLEGRO_EVENT event;				// allegro event received
	int keys[ALLEGRO_KEY_MAX];			// array of key state

	memset(keys, 0, sizeof(keys));

	ptask_activate(id);

	while (!done_local) {
		al_wait_for_event_timed(input_queue, &event, TASK_INPUT_EVENT_TIME);

		if (keys[ALLEGRO_KEY_ESCAPE]) {
			MUTEX_EXP(done = TRUE, mux_done);
		}

		if (keys[ALLEGRO_KEY_UP]) {
			MUTEX_LOCK(mux_bubble_scale);
			if (bubble_scale < BUBBLE_SCALE_MAX) {
				bubble_scale += BUBBLE_SCALE_STEP;
			}
			MUTEX_UNLOCK(mux_bubble_scale);
		}

		if (keys[ALLEGRO_KEY_DOWN]) {
			MUTEX_LOCK(mux_bubble_scale);
			if (bubble_scale > BUBBLE_SCALE_MIN) {
				bubble_scale -= BUBBLE_SCALE_STEP;
			}
			MUTEX_UNLOCK(mux_bubble_scale);
		}

		if (keys[ALLEGRO_KEY_LEFT]) {
			MUTEX_LOCK(mux_active_tasks);
			if (active_tasks > BUBBLE_TASKS_MIN) {
				active_tasks--;
			}
			MUTEX_UNLOCK(mux_active_tasks);
		}

		if (keys[ALLEGRO_KEY_RIGHT]) {
			MUTEX_LOCK(mux_active_tasks);
			if (active_tasks < BUBBLE_TASKS_MAX) {
				active_tasks++;
			}
			MUTEX_UNLOCK(mux_active_tasks);
		}

		if (keys[ALLEGRO_KEY_CLOSEBRACE]) {
			MUTEX_LOCK(mux_gain);
			if (gain < GAIN_MAX) {
				gain += GAIN_STEP;
				allegro_stream_set_gain(gain);
			}
			MUTEX_UNLOCK(mux_gain);
		}

		if (keys[ALLEGRO_KEY_SLASH]) {
			MUTEX_LOCK(mux_gain);
			if (gain > GAIN_MIN) {
				gain -= GAIN_STEP;
				allegro_stream_set_gain(gain);
			}
			MUTEX_UNLOCK(mux_gain);
		}

		// Reading numbers from 1 to 7
		for (i = ALLEGRO_KEY_1; i < ALLEGRO_KEY_8; ++i) {
			if (keys[i]) {
				MUTEX_LOCK(mux_windowing);
				windowing = i - ALLEGRO_KEY_0 - 1;
				MUTEX_UNLOCK(mux_windowing);
			}
		}

		if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
			keys[event.keyboard.keycode] = 1;
		}

		if (event.type == ALLEGRO_EVENT_KEY_UP) {
			keys[event.keyboard.keycode] = 0;
		}

		MUTEX_EXP(done_local = done, mux_done);

		if (ptask_deadline_miss(id) == PTASK_DEADLINE_MISS) {
			fprintf(stderr, "%zu) deadline missed! woet: %zu ms\n",
					id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}

//------------------------------------------------------------------------------
//
// DESCRIPTION
// This is the main function that calls all the functions needed to start the
// program.
//
// PARAMETERS
// argc: the number of parameters given to the program
// argv: the parameters given to the program
//
//------------------------------------------------------------------------------
int main(int argc, char * argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Please provide an audio filename.\n");
		exit(EXIT_NO_FILENAME);
	}

	sound2image_init_variables(argv[1]);
	sound2image_create_tasks();
	sound2image_waits_tasks();
	sound2image_free();

	return 0;
}
