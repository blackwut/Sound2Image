#include <stdio.h>
#include <stdlib.h> 
#include <pthread.h>
#include <math.h>
#include "common.h"
#include "fft_audio.h"
#include "allegro_utils.h"
#include "ptask.h"
#include "time_utils.h"
#include "btrails.h"

#define BUBBLES_INFO_TEXT_ALIGN ALLEGRO_ALIGN_LEFT
#define BUBBLES_INFO_X 10
#define BUBBLES_INFO_Y 0

#define TIMER_INFO_TEXT_ALIGN ALLEGRO_ALIGN_RIGHT
#define TIMER_INFO_X DISPLAY_W - 10
#define TIMER_INFO_Y 0

#define GAIN_INFO_TEXT_ALIGN ALLEGRO_ALIGN_RIGHT
#define GAIN_INFO_X DISPLAY_W - 10
#define GAIN_INFO_Y 20

#define USER_INFO_TEXT	"UP: bigger bubbles - DOWN: smaller bubbles - " \
						"LEFT: less bubbles - RIGHT: more bubbles"
#define USER_INFO_TEXT_ALIGN ALLEGRO_ALIGN_CENTER
#define USER_INFO_X DISPLAY_W / 2
#define USER_INFO_Y DISPLAY_H - 32

#define FRAGMENT_COUNT			8
#define FRAGMENT_SAMPLES		882 // Must be a diveder of both FRAGMENT_SAMPLERATE and 1000/FRAMERATE
#define FRAGMENT_DEPTH 			ALLEGRO_AUDIO_DEPTH_FLOAT32

#define BUBBLE_TASKS_MIN 8
#define BUBBLE_TASKS_BASE 24
#define BUBBLE_TASKS_MAX 32
#define BUBBLE_FILTER 0.5


#define BUBBLE_X_OFFSCREEN		-100.0f
#define BUBBLE_Y_OFFSCREEN		-100.0f
#define BUBBLE_THICKNESS     2
#define BUBBLE_RADIUS 		 8
#define BUBBLE_ALPHA_FILLED  200
#define BUBBLE_ALPHA_STROKE  255

#define BUBBLE_SCALE_MIN    0.5f
#define BUBBLE_SCALE_BASE   1.0f
#define BUBBLE_SCALE_MAX    2.0f
#define BUBBLE_SCALE_STEP   0.1f

#define GAIN_MIN 0
#define GAIN_BASE 50
#define GAIN_MAX 100
#define GAIN_STEP 1

size_t samplerate;

pthread_mutex_t lock_done;
int done;

pthread_mutex_t lock_active_bubbles;
size_t active_bubbles;

pthread_mutex_t lock_bubble_scale;
float bubble_scale;

pthread_mutex_t lock_audio_time_ms;
size_t audio_time_ms;

pthread_cond_t cond_fft_producer;
pthread_cond_t cond_fft_consumers;
pthread_mutex_t lock_fft;
int var_fft;

pthread_mutex_t lock_gain;
size_t gain;

ALLEGRO_AUDIO_STREAM * stream;
ALLEGRO_MIXER * mixer;

#define MAX_COLORS 32
unsigned char colors[MAX_COLORS][3] = {
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


float bubble_spacing_with(size_t bubbles)
{
	return DISPLAY_W / (float)(bubbles + 1);
}

void fill_stream_fragment()
{
	float * buffer = NULL;

	buffer = al_get_audio_stream_fragment(stream);
	if (buffer != NULL) {
		fft_audio_fill_buffer_data(buffer, FRAGMENT_SAMPLES);
		al_check(al_set_audio_stream_fragment(stream, buffer),
				 "al_set_audio_stream_fragment()");
	} else {
		DLOG("Fragment not inserted!\n", NULL);
	}
}

void * task_fft(void * arg)
{
	const int id = ptask_id(arg);
	int done_local = 0;
	int ret;

	ptask_activate(id);

	while (!done_local) {

		MUTEX_LOCK(lock_fft);
		while (var_fft > 0) {
			pthread_cond_wait(&cond_fft_producer, &lock_fft);
		}

		fill_stream_fragment();
		EXP_LOCK(audio_time_ms += TASK_FFT_PERIOD, lock_audio_time_ms);

		fft_audio_compute_fft();

		ret = fft_audio_load_next_window();
		if (ret == FFT_AUDIO_EOF) {
			DLOG("FFT_AUDIO_EOF", NULL);
			EXP_LOCK(done = 1, lock_done);
		}

		var_fft = BUBBLE_TASKS_MAX;
		pthread_cond_broadcast(&cond_fft_consumers);
		MUTEX_UNLOCK(lock_fft);

		EXP_LOCK(done_local = done, lock_done);

		if (ptask_deadline_miss(id) == PTASK_DEADLINE_MISS) {
			DLOG("%d) deadline missed! woet: %zu ms\n",
				id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}

void samples_range(const size_t id, const size_t n, size_t * from, size_t * to)
{
	float step = log2f(FRAGMENT_SAMPLES / 2 - n) / (n + 1);
	*from = (size_t)lroundf(powf(2, (id + 1) * step)) + id + 1;
	*to = (size_t)lround(powf(2, (id + 2) * step)) + id + 2;
}

float calculate_new_val(const size_t id, const float val_old, const size_t from, const size_t to)
{
	float val;
	float valMin;
	float valMax;
	fft_audio_stats audio_stats;
	fft_audio_stats frame_stats;

	audio_stats = fft_audio_get_stats();
	frame_stats = fft_audio_get_stats_samples(from, to);

	valMin = log2f(audio_stats.magMin);
	valMax = log2f(audio_stats.magMax);
	val = log2f(frame_stats.magMax);
	val = (val - valMin) / (valMax - valMin);

	if (isfinite(val)) {
		return val_old * BUBBLE_FILTER + (1.0f - BUBBLE_FILTER) * val;
	}

	return 0.0f;
}

void * task_bubble(void * arg)
{
	const int id = ptask_id(arg);
	int done_local = 0;
	size_t active_bubbles_local;
	float bubble_spacing;
	size_t from_sample;
	size_t to_sample;
	float val = 0.0f;
	float x = 0.0f;
	float y = 0.0f;
	size_t color_id;

	ptask_activate(id);

	while (!done_local) {

		EXP_LOCK(active_bubbles_local = active_bubbles, lock_active_bubbles);
		bubble_spacing = bubble_spacing_with(active_bubbles_local);

		MUTEX_LOCK(lock_fft);
		while (var_fft == 0) {
			pthread_cond_wait(&cond_fft_consumers, &lock_fft);
		}
		MUTEX_UNLOCK(lock_fft);

		if (id < active_bubbles_local) {
			samples_range(id, active_bubbles_local, &from_sample, &to_sample);
			val = calculate_new_val(id, val, from_sample, to_sample);
			color_id = MAX_COLORS * (id / (float) active_bubbles_local);
			x = (id + 1) * bubble_spacing;
			y = DISPLAY_AUDIO_Y + DISPLAY_AUDIO_H - val * DISPLAY_AUDIO_H;
		} else {
			to_sample = 0;
			color_id = 0;
			x = BUBBLE_X_OFFSCREEN;
			y = BUBBLE_Y_OFFSCREEN;
		}

		btrails_lock(id);
		btrails_set_color(id,
						  colors[color_id][0],
						  colors[color_id][1],
						  colors[color_id][2]);
		btrails_set_freq(id, to_sample * samplerate / FRAGMENT_SAMPLES);
		btrails_put_bubble_pos(id, x, y);
		btrails_unlock(id);

		MUTEX_LOCK(lock_fft);
		var_fft--;
		if (var_fft == 0) {
			pthread_cond_signal(&cond_fft_producer);
		}
		MUTEX_UNLOCK(lock_fft);

		EXP_LOCK(done_local = done, lock_done);

		if (ptask_deadline_miss(id) == PTASK_DEADLINE_MISS) {
			DLOG("%d) deadline missed! woet: %zu ms\n",
				id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}

void draw_trail(const size_t id, const float spacing, const float scale)
{
	size_t i;
	size_t top;
	size_t freq;
	BColor bcolor;
	ALLEGRO_COLOR color;
	float alpha;
	BPoint bpoint;

	btrails_lock(id);
	top = btrails_get_top(id);
	freq = btrails_get_freq(id);
	bcolor = btrails_get_color(id);

	allegro_blender_mode_standard();
	color = al_map_rgba(bcolor.red,
						bcolor.green,
						bcolor.blue,
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

		color = al_map_rgba(bcolor.red,
							bcolor.green,
							bcolor.blue,
							BUBBLE_ALPHA_FILLED * alpha);
		al_draw_filled_circle(bpoint.x, bpoint.y,
							  BUBBLE_RADIUS * scale,
							  color);

		color = al_map_rgba(bcolor.red,
							bcolor.green,
							bcolor.blue,
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
	size_t active_bubbles_local;
	float spacing;
	float scale;

	EXP_LOCK(active_bubbles_local = active_bubbles, lock_active_bubbles);
	EXP_LOCK(scale = bubble_scale, lock_bubble_scale);

	spacing = bubble_spacing_with(active_bubbles_local);

	for (i = 0; i < active_bubbles_local; ++i) {
		draw_trail(i, spacing, scale);
	}
}

void draw_bubbles_info()
{
	size_t active_bubbles_local;

	EXP_LOCK(active_bubbles_local = active_bubbles, lock_active_bubbles);

	allegro_blender_mode_standard();
	al_draw_textf(font_big, font_color,
				  BUBBLES_INFO_X, BUBBLES_INFO_Y,
				  BUBBLES_INFO_TEXT_ALIGN,
				  "Bubbles: %zu", active_bubbles_local);
}

void draw_timer_info()
{
	size_t time_ms;
	size_t minutes;
	size_t seconds;
	size_t milliseconds;

	EXP_LOCK(time_ms = audio_time_ms, lock_audio_time_ms);

	minutes = TIME_MSEC_TO_SEC(time_ms) / 60;
	seconds = TIME_MSEC_TO_SEC(time_ms) % 60;
	milliseconds = time_ms % TIME_MILLISEC;

	allegro_blender_mode_standard();
	al_draw_textf(font_big, font_color,
				  TIMER_INFO_X, TIMER_INFO_Y,
				  TIMER_INFO_TEXT_ALIGN,
				  "%02zu:%02zu:%03zu", minutes, seconds, milliseconds);
}

void draw_gain_info()
{
	size_t gain_local;

	EXP_LOCK(gain_local = gain, lock_gain);

	al_draw_textf(font_big, font_color,
				  GAIN_INFO_X, GAIN_INFO_Y,
				  GAIN_INFO_TEXT_ALIGN,
				  "Vol:%3zu", gain_local);
}

void draw_user_info()
{
	al_draw_textf(font_big, font_color,
				  USER_INFO_X, USER_INFO_Y,
				  USER_INFO_TEXT_ALIGN,
				  USER_INFO_TEXT);
}

void * task_display(void * arg)
{
	const int id = ptask_id(arg);
	int done_local = 0;

	al_set_target_backbuffer(display);

	ptask_activate(id);

	while (!done_local) {

		al_clear_to_color(BACKGROUND_COLOR);

		draw_trails();
		draw_bubbles_info();
		draw_timer_info();
		draw_gain_info();
		draw_user_info();

		al_flip_display();

		EXP_LOCK(done_local = done, lock_done);

		if (ptask_deadline_miss(id) == PTASK_DEADLINE_MISS) {
			DLOG("%d) deadline missed! woet: %zu ms\n",
				id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}

void update_gain(size_t val)
{
	al_check(al_set_audio_stream_gain(stream, val / 100.0f),
				 	 				  "al_set_audio_stream_gain()");
}

void * task_input(void * arg)
{
	const int id = ptask_id(arg);
	ALLEGRO_EVENT event;
	int done_local = 0;
	int keys[ALLEGRO_KEY_MAX];
	memset(keys, 0, sizeof(keys));

	ptask_activate(id);

	while (!done_local) {
		memset(&event, 0, sizeof(ALLEGRO_EVENT));
		al_wait_for_event_timed(input_queue, &event, TASK_INPUT_EVENT_TIME);

		if (keys[ALLEGRO_KEY_ESCAPE]) {
			EXP_LOCK(done = 1, lock_done);
		}

		if (keys[ALLEGRO_KEY_UP]) {
			MUTEX_LOCK(lock_bubble_scale);
			if (bubble_scale < BUBBLE_SCALE_MAX) {
				bubble_scale += BUBBLE_SCALE_STEP;
			}
			MUTEX_UNLOCK(lock_bubble_scale);
		}

		if (keys[ALLEGRO_KEY_DOWN]) {
			MUTEX_LOCK(lock_bubble_scale);
			if (bubble_scale > BUBBLE_SCALE_MIN) {
				bubble_scale -= BUBBLE_SCALE_STEP;
			}
			MUTEX_UNLOCK(lock_bubble_scale);
		}

		if (keys[ALLEGRO_KEY_LEFT]) {
			MUTEX_LOCK(lock_active_bubbles);
			if (active_bubbles > BUBBLE_TASKS_MIN) {
				active_bubbles--;
			}
			MUTEX_UNLOCK(lock_active_bubbles);
		}

		if (keys[ALLEGRO_KEY_RIGHT]) {
			MUTEX_LOCK(lock_active_bubbles);
			if (active_bubbles < BUBBLE_TASKS_MAX) {
				active_bubbles++;
			}
			MUTEX_UNLOCK(lock_active_bubbles);
		}

		if (keys[ALLEGRO_KEY_CLOSEBRACE]) {
			MUTEX_LOCK(lock_gain);
			if (gain < GAIN_MAX) {
				gain += GAIN_STEP;
				update_gain(gain);
			}
			MUTEX_UNLOCK(lock_gain);
		}

		if (keys[ALLEGRO_KEY_SLASH]) {
			MUTEX_LOCK(lock_gain);
			if (gain > GAIN_MIN) {
				gain -= GAIN_STEP;
				update_gain(gain);
			}
			MUTEX_UNLOCK(lock_gain);
		}

		if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
			keys[event.keyboard.keycode] = 1;
		}

		if (event.type == ALLEGRO_EVENT_KEY_UP) {
			keys[event.keyboard.keycode] = 0;
		}

		EXP_LOCK(done_local = done, lock_done);

		if (ptask_deadline_miss(id) == PTASK_DEADLINE_MISS) {
			DLOG("%d) deadline missed! woet: %zu ms\n",
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

	pthread_mutex_init(&lock_done, NULL);
	pthread_mutex_init(&lock_audio_time_ms, NULL);
	pthread_mutex_init(&lock_bubble_scale, NULL);
	pthread_mutex_init(&lock_active_bubbles, NULL);

	pthread_cond_init(&cond_fft_producer, NULL);
	pthread_cond_init(&cond_fft_consumers, NULL);
	pthread_mutex_init(&lock_fft, NULL);
	pthread_mutex_init(&lock_gain, NULL);

	done = 0;
	var_fft = 0;
	active_bubbles = BUBBLE_TASKS_BASE;
	bubble_scale = BUBBLE_SCALE_BASE;
	audio_time_ms = 0;
	gain = GAIN_BASE;
	memset(audio_filename, 0, FILENAME_SIZE);

	if (argc < 2) {
		printf("Please provide an audio filename.\n");
		exit(EXIT_NO_FILENAME_PROVIDED);
	}
	strncpy(audio_filename, argv[1], FILENAME_SIZE);
	
	ret = fft_audio_init(audio_filename, FRAGMENT_SAMPLES, fft_audio_hamming);
	if (ret == FFT_AUDIO_ERROR_OPEN_FILE) {
		printf("Cannot open file %s\n", audio_filename);
		exit(EXIT_OPEN_FILE);
	}
	samplerate = fft_audio_get_samplerate();
	channels = fft_audio_get_channels();

	allegro_init();
	al_set_target_bitmap(NULL);

	mixer = al_get_default_mixer();
	stream = al_create_audio_stream(FRAGMENT_COUNT,
									FRAGMENT_SAMPLES,
									samplerate,
									ALLEGRO_AUDIO_DEPTH_FLOAT32,
									allegro_channel_conf_with(channels));
	al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_ONCE);
	update_gain(gain);
	al_check(al_attach_audio_stream_to_mixer(stream, mixer),
			 "al_attach_audio_stream_to_mixer()");

	ret = btrails_init();
	if (ret == BTRAILS_ERROR) {
		printf("Cannot create Bubble Trails.\n");
		exit(EXIT_BTRAILS_ERROR);
	}

	// Tasks
	for (int i = 0; i < BUBBLE_TASKS_MAX; ++i) {
		ptask_create(task_bubble,
					 TASK_BUBBLE_PERIOD,
					 TASK_BUBBLE_DEADLINE,
					 TASK_BUBBLE_PRIORITY);
	}

	ptask_create(task_fft,
				 TASK_FFT_PERIOD,
				 TASK_FFT_DEADLINE,
				 TASK_FFT_PRIORITY);
	ptask_create(task_input,
				 TASK_INPUT_PERIOD,
				 TASK_INPUT_DEADLINE,
				 TASK_INPUT_PRIORITY);
	ptask_create(task_display,
				 TASK_DISPLAY_PERIOD,
				 TASK_DISPLAY_DEADLINE,
				 TASK_DISPLAY_PRIORITY);
	ptask_wait_tasks();

	// Free
	pthread_mutex_destroy(&lock_active_bubbles);
	pthread_mutex_destroy(&lock_bubble_scale);
	pthread_mutex_destroy(&lock_audio_time_ms);
	pthread_mutex_destroy(&lock_done);

	pthread_cond_destroy(&cond_fft_producer);
	pthread_cond_destroy(&cond_fft_consumers);
	pthread_mutex_destroy(&lock_fft);

	pthread_mutex_destroy(&lock_gain);

	btrails_free();
	fft_audio_free();
	al_drain_audio_stream(stream);
	allegro_free();

	return 0;
}
