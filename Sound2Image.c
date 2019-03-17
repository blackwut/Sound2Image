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
#include "allegro_utils.h"
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
float bubble_spacing_with(size_t n);
fft_audio_range bubble_samples_range(const size_t id,
									 const size_t n);
float bubble_calculate_val(const size_t id,
						   const float val_old,
						   const fft_audio_range range);
void stream_set_gain(size_t val);
void stream_fill_new_fragment();
void draw_trail(const size_t id,
				const float spacing,
				const float scale);
void draw_trails();
void draw_bubbles_info();
void draw_windowing_info();
void draw_time_info();
void draw_gain_info();
void draw_user_info();
void * task_fft(void * arg);
void * task_bubble(void * arg);
void * task_display(void * arg);
void * task_input(void * arg);

//------------------------------------------------------------------------------
// SOUND2IMAGE GLOBAL DATA
//------------------------------------------------------------------------------
size_t samplerate;					// Samplerate of audio file.
int done;							// If TRUE the program stops
size_t active_tasks;				// Num. of tasks active
size_t gain;						// Volume of audio listening
fft_audio_windowing windowing;		// Windowing method of FFT
size_t time_ms;						// Elapsed time of audio listening
float bubble_scale;					// Scale factor of displayed bubbles
ALLEGRO_AUDIO_STREAM * stream;		// Audio stream object

// Mutexes
pthread_mutex_t mux_done;			// Mutex associated to variable done
pthread_mutex_t mux_active_tasks;	// Mutex associated to variable active_tasks
pthread_mutex_t mux_gain;			// Mutex associated to variable gain
pthread_mutex_t mux_windowing;		// Mutex associated to variable windowing
pthread_mutex_t mux_time_ms;		// Mutex associated to variable time_ms
pthread_mutex_t mux_bubble_scale;	// Mutex associated to variable bubble_scale

//------------------------------------------------------------------------------
// Mutex, Condition variables and counter to implement precedence relationship
// between task_fft and all task_bubble periodic tasks.
//------------------------------------------------------------------------------
pthread_cond_t cond_fft_producer;	// Cond. var. associated to task_fft
pthread_cond_t cond_fft_consumers;	// Cond. var. associated to all task_bubble
pthread_mutex_t mux_fft;			// Mutex associated to the prev. cond. var.
size_t fft_counter;					// Variable to make synchronization


//------------------------------------------------------------------------------
// SOUND2IMAGE FUNCTION DEFINITIONS
//------------------------------------------------------------------------------

// void allegro_check(bool test, const char * description)
// {
//     if (test) return;
//     fprintf(stderr, "ALLEGRO ERROR - %s\n", description);
//     exit(EXIT_ALLEGRO_ERROR);
// }

void ptask_check(int ret, const char * description)
{
	if (ret == PTASK_SUCCESS) return;
	fprintf(stderr, "PTASK ERROR - %s\n", description);

	if (ret == PTASK_ERROR_CREATE) exit(EXIT_PTASK_CREATE);
	if (ret == PTASK_ERROR_JOIN) exit(EXIT_PTASK_JOIN);
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
//
//------------------------------------------------------------------------------
fft_audio_range bubble_samples_range(const size_t id,
									 const size_t n)
{
	float step;
	fft_audio_range range;

	step = log2f(FRAGMENT_SAMPLES / 2 - n) / (n + 1);
	range.from = (size_t)lroundf(powf(2, (id + 1) * step)) + id + 1;
	range.to = (size_t)lround(powf(2, (id + 2) * step)) + id + 2;

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
	float val;
	float valMin;
	float valMax;
	fft_audio_stats audio_stats;
	fft_audio_stats frame_stats;

	audio_stats = fft_audio_get_stats();
	frame_stats = fft_audio_get_stats_samples(range);

	valMin = log2f(audio_stats.magMin);
	valMax = log2f(audio_stats.magMax);
	val = log2f(frame_stats.magMax);
	val = (val - valMin) / (valMax - valMin);

	if (isfinite(val)) {
		return val_old * BUBBLE_FILTER + (1.0f - BUBBLE_FILTER) * val;
	}

	return 0.0f;
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
void stream_set_gain(size_t val)
{
	allegro_check(al_set_audio_stream_gain(stream, val / 100.0f),
				  "al_set_audio_stream_gain()");
}

//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function copies the current frame audio values to a buffer provided by
// the streaming object.
//
//------------------------------------------------------------------------------
void stream_fill_new_fragment()
{
	float * buffer = NULL;

	buffer = al_get_audio_stream_fragment(stream);
	if (buffer != NULL) {
		fft_audio_fill_buffer_data(buffer);
		allegro_check(al_set_audio_stream_fragment(stream, buffer),
					  "al_set_audio_stream_fragment()");
	} else {
		fprintf(stderr, "Fragment not inserted!\n", NULL);
	}
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
	size_t i;
	size_t top;
	size_t freq;
	btrail_color bcolor;
	btrail_point bpoint;
	float alpha;
	ALLEGRO_COLOR color;

	btrails_lock(id);
	top = btrails_get_top(id);
	freq = btrails_get_freq(id);
	bcolor = btrails_get_color(id);

	allegro_blender_mode_standard();
	color = al_map_rgba(bcolor.red, bcolor.green, bcolor.blue,
						BUBBLE_ALPHA_STROKE);
	al_draw_textf(font_small, color,
				  (id + 1) * spacing,
				  DISPLAY_AUDIO_H + DISPLAY_AUDIO_Y + 20,
				  ALLEGRO_ALIGN_CENTER,
				  "%zu", freq);

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

void draw_trails()
{
	size_t i;
	size_t active_tasks_local;
	float spacing;
	float scale;

	MUTEX_EXP(active_tasks_local = active_tasks, mux_active_tasks);
	MUTEX_EXP(scale = bubble_scale, mux_bubble_scale);

	spacing = bubble_spacing_with(active_tasks_local);

	for (i = 0; i < active_tasks_local; ++i) {
		draw_trail(i, spacing, scale);
	}
}

void draw_bubbles_info()
{
	size_t active_tasks_local;

	MUTEX_EXP(active_tasks_local = active_tasks, mux_active_tasks);

	allegro_blender_mode_standard();
	al_draw_textf(font_big, font_color,
				  BUBBLES_INFO_X, BUBBLES_INFO_Y,
				  BUBBLES_INFO_TEXT_ALIGN,
				  "Bubbles: %zu", active_tasks_local);
}

void draw_windowing_info()
{
	int windowing_local;

	MUTEX_EXP(windowing_local = windowing, mux_windowing);
	fft_audio_get_windowing_name(windowing_local);

	allegro_blender_mode_standard();
	al_draw_textf(font_big, font_color,
				  WINDOWING_INFO_X, WINDOWING_INFO_Y,
				  WINDOWING_INFO_TEXT_ALIGN,
				  "Windowing: %s",
				  fft_audio_get_windowing_name(windowing_local));
}

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

void draw_gain_info()
{
	size_t gain_local;

	MUTEX_EXP(gain_local = gain, mux_gain);

	al_draw_textf(font_big, font_color,
				  GAIN_INFO_X, GAIN_INFO_Y,
				  GAIN_INFO_TEXT_ALIGN,
				  "Vol:%3zu", gain_local);
}

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
	const int id = ptask_id(arg);		// id of this periodic task
	int done_local = FALSE;					// local value of done
	int windowing_local;				// local value of windowing method
	int ret;							// ret value

	// Activate for the first time this periodic task
	ptask_activate(id);

	while (!done_local) {

		// Wait the computation of all task_bubble
		MUTEX_LOCK(mux_fft);
		while (fft_counter > 0) {
			pthread_cond_wait(&cond_fft_producer, &mux_fft);
		}

		// Provide new values to the audio stream
		stream_fill_new_fragment();
		// Update the elapsed audio time
		MUTEX_EXP(time_ms += FRAGMENT_SAMPLES * 1000/ samplerate,
				 mux_time_ms);

		// Load the curent windowing method and compute the FFT
		MUTEX_EXP(windowing_local = windowing, mux_windowing);
		fft_audio_compute_fft(windowing_local);

		// Load a new frame for the next period
		ret = fft_audio_load_next_frame();
		if (ret == FFT_AUDIO_EOF) {
			MUTEX_EXP(done = TRUE, mux_done);
		}

		// Awake all task_bubble to make them computing the new frame
		fft_counter = BUBBLE_TASKS_MAX;
		pthread_cond_broadcast(&cond_fft_consumers);
		MUTEX_UNLOCK(mux_fft);

		// Update the current status of done variable
		MUTEX_EXP(done_local = done, mux_done);

		// Check of a deadline miss and wait for the next period
		if (ptask_deadline_miss(id) == PTASK_DEADLINE_MISS) {
			fprintf(stderr, "%d) deadline missed! woet: %zu ms\n",
				id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}


void * task_bubble(void * arg)
{
	const int id = ptask_id(arg);
	int done_local = FALSE;
	size_t active_tasks_local;
	float bubble_spacing;
	fft_audio_range range;
	float val = 0.0f;
	float x = 0.0f;
	float y = 0.0f;
	size_t color_id;

	ptask_activate(id);

	while (!done_local) {

		// Wait the computation of task_fft
		MUTEX_LOCK(mux_fft);
		while (fft_counter == 0) {
			pthread_cond_wait(&cond_fft_consumers, &mux_fft);
		}
		MUTEX_UNLOCK(mux_fft);

		// Update the local value of active_tasks and calculate the spacing
		MUTEX_EXP(active_tasks_local = active_tasks, mux_active_tasks);
		bubble_spacing = bubble_spacing_with(active_tasks_local);

		if (id < active_tasks_local) {
			range = bubble_samples_range(id, active_tasks_local);
			val = bubble_calculate_val(id, val, range);
			color_id = MAX_COLORS * (id / (float) active_tasks_local);
			x = (id + 1) * bubble_spacing;
			y = DISPLAY_AUDIO_Y + DISPLAY_AUDIO_H - val * DISPLAY_AUDIO_H;
		} else {
			range.from = 0;
			range.to = 0;
			color_id = 0;
			x = BUBBLE_X_OFFSCREEN;
			y = BUBBLE_Y_OFFSCREEN;
		}

		// Put a new bubble into the circular buffer
		btrails_lock(id);
		btrails_set_color(id,
						  colors[color_id][0],
						  colors[color_id][1],
						  colors[color_id][2]);
		btrails_set_freq(id, range.to * samplerate / FRAGMENT_SAMPLES);
		btrails_put_bubble_pos(id, x, y);
		btrails_unlock(id);

		// Awake the task_fft if all task_bubble completed the computation
		MUTEX_LOCK(mux_fft);
		fft_counter--;
		if (fft_counter == 0) {
			pthread_cond_signal(&cond_fft_producer);
		}
		MUTEX_UNLOCK(mux_fft);

		// Update the current status of done variable
		MUTEX_EXP(done_local = done, mux_done);

		// Check of a deadline miss and wait for the next period
		if (ptask_deadline_miss(id) == PTASK_DEADLINE_MISS) {
			fprintf(stderr, "%d) deadline missed! woet: %zu ms\n",
				id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}



void * task_display(void * arg)
{
	const int id = ptask_id(arg);		// id of this periodic task
	int done_local = FALSE;					// local value of done

	// Set the target buffer of drawing functions
	al_set_target_backbuffer(display);

	ptask_activate(id);

	while (!done_local) {

		al_clear_to_color(BACKGROUND_COLOR);

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
			fprintf(stderr, "%d) deadline missed! woet: %zu ms\n",
				id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}

void * task_input(void * arg)
{
	const int id = ptask_id(arg);
	int done_local = FALSE;
	size_t i;
	ALLEGRO_EVENT event;
	int keys[ALLEGRO_KEY_MAX];

	memset(keys, 0, sizeof(keys));

	ptask_activate(id);

	while (!done_local) {
		memset(&event, 0, sizeof(ALLEGRO_EVENT));
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
				stream_set_gain(gain);
			}
			MUTEX_UNLOCK(mux_gain);
		}

		if (keys[ALLEGRO_KEY_SLASH]) {
			MUTEX_LOCK(mux_gain);
			if (gain > GAIN_MIN) {
				gain -= GAIN_STEP;
				stream_set_gain(gain);
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
			fprintf(stderr, "%d) deadline missed! woet: %zu ms\n",
				id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}

int main(int argc, char * argv[])
{
	char audio_filename[FILENAME_SIZE];
	size_t channels;
	int ret;

	pthread_mutex_init(&mux_done, NULL);
	pthread_mutex_init(&mux_time_ms, NULL);
	pthread_mutex_init(&mux_bubble_scale, NULL);
	pthread_mutex_init(&mux_active_tasks, NULL);

	pthread_cond_init(&cond_fft_producer, NULL);
	pthread_cond_init(&cond_fft_consumers, NULL);
	pthread_mutex_init(&mux_fft, NULL);
	pthread_mutex_init(&mux_gain, NULL);
	pthread_mutex_init(&mux_windowing, NULL);

	done = FALSE;
	fft_counter = 0;
	active_tasks = BUBBLE_TASKS_BASE;
	bubble_scale = BUBBLE_SCALE_BASE;
	time_ms = 0;
	gain = GAIN_BASE;
	windowing = fft_audio_rectangular;
	memset(audio_filename, 0, FILENAME_SIZE);

	if (argc < 2) {
		fprintf(stderr, "Please provide an audio filename.\n");
		exit(EXIT_NO_FILENAME);
	}
	strncpy(audio_filename, argv[1], FILENAME_SIZE);
	
	ret = fft_audio_init(audio_filename, FRAGMENT_SAMPLES, fft_audio_hamming);
	if (ret != FFT_AUDIO_SUCCESS) {
		fprintf(stderr, "File does not exits or it is not compatible\n");
		exit(EXIT_OPEN_FILE);
	}
	samplerate = fft_audio_get_samplerate();
	channels = fft_audio_get_channels();

	allegro_init();
	al_set_target_bitmap(NULL);

	stream = al_create_audio_stream(FRAGMENT_COUNT,
									FRAGMENT_SAMPLES,
									samplerate,
									ALLEGRO_AUDIO_DEPTH_FLOAT32,
									allegro_channel_conf_with(channels));
	al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_ONCE);
	stream_set_gain(gain);
	allegro_check(al_attach_audio_stream_to_mixer(stream, 
												  al_get_default_mixer()),
				  "al_attach_audio_stream_to_mixer()");

	ret = btrails_init();
	if (ret == BTRAILS_ERROR) {
		printf("Cannot create Bubble Trails.\n");
		exit(EXIT_BTRAILS_ERROR);
	}

	// Tasks
	for (int i = 0; i < BUBBLE_TASKS_MAX; ++i) {
		ptask_check(ptask_create(task_bubble,
								 TASK_BUBBLE_PERIOD,
								 TASK_BUBBLE_DEADLINE,
								 TASK_BUBBLE_PRIORITY),
					"task_bubble");
	}

	ptask_check(ptask_create(task_fft,
							 TASK_FFT_PERIOD,
							 TASK_FFT_DEADLINE,
							 TASK_FFT_PRIORITY),
				"task_fft");
	ptask_check(ptask_create(task_input,
							 TASK_INPUT_PERIOD,
							 TASK_INPUT_DEADLINE,
							 TASK_INPUT_PRIORITY),
				"task_input");
	ptask_check(ptask_create(task_display,
							 TASK_DISPLAY_PERIOD,
							 TASK_DISPLAY_DEADLINE,
							 TASK_DISPLAY_PRIORITY),
				"task_display");
	
	ptask_check(ptask_wait_tasks(), "ptask_wait_tasks()");

	// Free
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

	return 0;
}
