#include <stdio.h>
#include <stdlib.h> 
#include <pthread.h>
#include <semaphore.h>
#include <math.h>
#include "common.h"
#include "fft_audio.h"
#include "allegro_utils.h"
#include "ptask.h"
#include "time_utils.h"
#include "bubble_trails.h"

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

#define FRAGMENT_SAMPLERATE		44100
#define FRAGMENT_COUNT			8
#define FRAGMENT_SIZE			882 // Must be a diveder of both FRAGMENT_SAMPLERATE and 1000/FRAMERATE
#define FRAGMENT_DEPTH 			ALLEGRO_AUDIO_DEPTH_FLOAT32

#define BUBBLE_TASKS_MIN 8
#define BUBBLE_TASKS_BASE 24
#define BUBBLE_TASKS_MAX 32
#define BUBBLE_FILTER_PARAM 0.5


#define BUBBLE_THICKNESS     2
#define BUBBLE_ALPHA_FILLED  200
#define BUBBLE_ALPHA         255

#define BUBBLE_SCALE_MIN    0.5f
#define BUBBLE_SCALE_BASE   1.0f
#define BUBBLE_SCALE_MAX    2.0f
#define BUBBLE_SCALE_STEP   0.1f

pthread_mutex_t lock_bubble_scale;
float BUBBLE_SCALE = BUBBLE_SCALE_BASE;

unsigned char colors[32][3] = {
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

pthread_cond_t cond_producer;
pthread_cond_t cond_consumers;
pthread_mutex_t lock_fft;
int var_fft;


pthread_mutex_t lock_bubble_tasks;
size_t bubble_tasks;

pthread_mutex_t lock_done;
int DONE;

#define GAIN_MIN 0
#define GAIN_BASE 50
#define GAIN_MAX 100
#define GAIN_STEP 1

pthread_mutex_t lock_gain;
size_t GAIN;

pthread_mutex_t lock_audio_time_ms;
size_t audio_time_ms;

fft_audio audio; //TODO: LOCK
fft_audio_stats audio_stats[BUBBLE_TASKS_MAX];

ALLEGRO_AUDIO_STREAM * stream;
ALLEGRO_MIXER * mixer;

void draw_bubble(Bubble b, float alpha) //TODO: separate draw of bubbles from draw of text. This can be done removing RGB and freq from bubble struct
{
	float bubble_scale_local;

	EXP_LOCK(bubble_scale_local = BUBBLE_SCALE, lock_bubble_scale);

	const float radius	= b.radius * bubble_scale_local;
	const float x		= (radius == 0 ? -radius : b.x);
	const float y		= (radius == 0 ? -radius : b.y);
	char buffer[8];

	ALLEGRO_COLOR color_font = al_map_rgba(b.red,
										   b.green,
										   b.blue,
										   255);
	ALLEGRO_COLOR color_filled = al_map_rgba(b.red,
											 b.green,
											 b.blue,
											 BUBBLE_ALPHA_FILLED * alpha);
	ALLEGRO_COLOR color_circle = al_map_rgba(b.red,
											 b.green,
											 b.blue,
											 BUBBLE_ALPHA * alpha);

	snprintf(buffer, 8, "%zu", b.freq);

	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	al_draw_filled_circle(x, y, radius, color_filled);
	al_draw_circle(x, y, radius, color_circle, 1.0f);
	allegro_print_text_small_color(buffer, color_font,
								   x, DISPLAY_AUDIO_H + DISPLAY_AUDIO_Y + 20,
								   ALLEGRO_ALIGN_CENTER);
}

void fill_stream_fragment()
{
	size_t i;
	float * buffer = NULL;

	buffer = al_get_audio_stream_fragment(stream);
	if (buffer != NULL) {
		for (i = 0; i < FRAGMENT_SIZE * audio.channels; ++i) {
			buffer[i] = audio.data[i];
		}
		al_check(al_set_audio_stream_fragment(stream, buffer),
				 "al_set_audio_stream_fragment()");
	} else {
		DLOG("Fragment not inserted!\n", NULL);
	}
}

void * task_fft(void * arg) //TODO: implement syncrhonization between task_fft and task_bubble
{
	const int id = ptask_id(arg);
	int done_local = 0;
	int ret;
	ptask_activate(id);

	while (!done_local) {

		MUTEX_LOCK(lock_fft);
		while (var_fft > 0) {
			pthread_cond_wait(&cond_producer, &lock_fft);
		}

		fill_stream_fragment();

		ret = fft_audio_next_window(&audio);
		if (ret == FFT_AUDIO_EOF) {
			EXP_LOCK(DONE += 1, lock_done);
		}

		var_fft = BUBBLE_TASKS_MAX;
		pthread_cond_broadcast(&cond_consumers);
		MUTEX_UNLOCK(lock_fft);

		EXP_LOCK(audio_time_ms += TASK_FFT_PERIOD, lock_audio_time_ms);
		EXP_LOCK(done_local = DONE, lock_done);

		if (ptask_deadline_miss(id) == PTASK_DEADLINE_MISS) {
			DLOG("%d) deadline missed! woet: %zu ms\n", id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}

void * task_bubble(void * arg)
{
	const int id = ptask_id(arg);
	int done_local = 0;
	float bubble_spacing;
	size_t from_sample = 0;
	size_t to_sample = 0;
	float y = 0.0f;
	size_t bubble_tasks_local;

	Bubble bubble;

	ptask_activate(id);

	while (!done_local) {

		EXP_LOCK(bubble_tasks_local = bubble_tasks, lock_bubble_tasks);

		MUTEX_LOCK(lock_fft);
		while (var_fft == 0) {
			pthread_cond_wait(&cond_consumers, &lock_fft);
		}

		if (id < bubble_tasks_local) {
			//TODO: check rounding and out of bound (if excedes FRAGMENT_SIZE / 2)
			float step = log2f(FRAGMENT_SIZE / 2 - bubble_tasks_local) / (bubble_tasks_local + 1);
			size_t from = lroundf(powf(2, (id + 1) * step)) + id + 1;
			size_t to = lroundf(powf(2, (id + 2) * step)) + id + 2;
			from_sample = from;
			to_sample = to;
			fft_audio_stats_samples(&(audio_stats[id]),
									&audio,
									from_sample,
									to_sample);

			bubble_spacing	= ((float)DISPLAY_W / (float)(bubble_tasks_local + 1));


			float audioMagMax = audio.stats.magMax;
			float audioMagMin = audio.stats.magMin;

			float windowMagMax = audio_stats[id].magMax;


			float audiodBMax = 20 * log2f(audioMagMax);
			float audiodBMin = 20 * log2f(audioMagMin);

			float windowdBMax = 20 * log2f(windowMagMax);

			float value = (windowdBMax - audiodBMin) / (audiodBMax - audiodBMin);

			if (isfinite(value)) {
				y = y * BUBBLE_FILTER_PARAM + (1.0f - BUBBLE_FILTER_PARAM) * value;
			}

			// //y = audio_stats[id].dB / 150.f;
			// if (isfinite(audio_stats[id].dB)) {
			// 	y = y * BUBBLE_FILTER_PARAM + (1.0f - BUBBLE_FILTER_PARAM) * audio_stats[id].dB / 150.f;
			// }

			bubble.radius	= 8;
			bubble.x		= (id + 1) * bubble_spacing;
			bubble.y		= DISPLAY_AUDIO_Y + DISPLAY_AUDIO_H - y * DISPLAY_AUDIO_H;
			bubble.red		= colors[id % 32][0];
			bubble.green	= colors[id % 32][1];
			bubble.blue		= colors[id % 32][2];
			bubble.freq		= to_sample * FRAGMENT_SAMPLERATE / FRAGMENT_SIZE;

		} else {
			bubble.radius	= 0;
			bubble.x		= 0;
			bubble.y		= 0;
			bubble.red		= 0;
			bubble.green	= 0;
			bubble.blue		= 0;
			bubble.freq		= 0;
		}
		BubbleTrails_put(id, bubble);

		var_fft--;
		if (var_fft == 0) {
			pthread_cond_signal(&cond_producer);
		}
		MUTEX_UNLOCK(lock_fft);

		EXP_LOCK(done_local = DONE, lock_done);

		if (ptask_deadline_miss(id) == PTASK_DEADLINE_MISS) {
			DLOG("%d) deadline missed! woet: %zu ms\n", id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}


void * task_display(void * arg)
{
	const int id = ptask_id(arg);
	int done_local = 0;
	Bubble b;
	size_t top;
	float alpha;
	size_t i;
	size_t j;
	size_t audio_time_ms_local;
	size_t minutes;
	size_t seconds;
	size_t milliseconds;
	char bubbles_text[BUBBLES_TEXT_SIZE];
	char timer_text[TIMER_TEXT_SIZE];
	char gain_text[GAIN_TEXT_SIZE];
	size_t gain_local;
	size_t bubble_tasks_local;

	memset(timer_text, 0, TIMER_TEXT_SIZE);
	al_set_target_backbuffer(display);

	ptask_activate(id);

	while (!done_local) {
		al_clear_to_color(BACKGROUND_COLOR);

		EXP_LOCK(bubble_tasks_local = bubble_tasks, lock_bubble_tasks);

		for (i = 0; i < bubble_tasks_local; ++i) {
			
			BubbleTrails_lock(i);
			top = BubbleTrails_top_unsafe(i);

			for (j = 0; j < MAX_BUBBLES_IN_TRAIL; ++j) {
				top = NEXT_BUBBLE(top);
				b = BubbleTrails_get_bubble(i, top);
				alpha = (MAX_BUBBLES_IN_TRAIL - j) / (float)MAX_BUBBLES_IN_TRAIL;
				draw_bubble(b, alpha);
			}

			BubbleTrails_unlock(i);
		}

		EXP_LOCK(audio_time_ms_local = audio_time_ms, lock_audio_time_ms);
		EXP_LOCK(gain_local = GAIN, lock_gain);

		minutes = TIME_MSEC_TO_SEC(audio_time_ms_local) / 60;
		seconds = TIME_MSEC_TO_SEC(audio_time_ms_local) % 60;
		milliseconds = audio_time_ms_local % TIME_MILLISEC;
		snprintf(bubbles_text, BUBBLES_TEXT_SIZE, "Bubbles: %zu", bubble_tasks_local);
		snprintf(timer_text, TIMER_TEXT_SIZE, "%02zu:%02zu:%03zu", minutes, seconds, milliseconds);
		snprintf(gain_text, GAIN_TEXT_SIZE, "Vol:%3zu", gain_local);
		allegro_print_text(bubbles_text,
						   BUBBLES_INFO_X, BUBBLES_INFO_Y,
						   BUBBLES_INFO_TEXT_ALIGN);
		allegro_print_text(timer_text,
						   TIMER_INFO_X, TIMER_INFO_Y,
						   TIMER_INFO_TEXT_ALIGN);
		allegro_print_text(USER_INFO_TEXT,
						   USER_INFO_X, USER_INFO_Y,
						   USER_INFO_TEXT_ALIGN);
		allegro_print_text(gain_text,
						   GAIN_INFO_X, GAIN_INFO_Y,
						   GAIN_INFO_TEXT_ALIGN);
		al_flip_display();


		EXP_LOCK(done_local = DONE, lock_done);

		if (ptask_deadline_miss(id) == PTASK_DEADLINE_MISS) {
			DLOG("%d) deadline missed! woet: %zu ms\n", id, ptask_get_woet_ms(id));
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}

void update_gain_unsafe()
{
	al_check(al_set_audio_stream_gain(stream, GAIN / 100.0f),
				 	 				  "al_set_audio_stream_gain()");
}

void * task_input(void * arg)
{
	ALLEGRO_EVENT event;
	int done_local = 0;
	int keycode;
	int keys[S2I_KEYS_ELEMENTS] = {0};

	while (!done_local) {
		memset(&event, 0, sizeof(ALLEGRO_EVENT));
		al_wait_for_event(input_queue, &event);

		if (keys[S2I_KEY_ESCAPE]) {
			EXP_LOCK(DONE += 1, lock_done);
		}

		if (keys[S2I_KEY_UP]) {
			MUTEX_LOCK(lock_bubble_scale);
			if (BUBBLE_SCALE < BUBBLE_SCALE_MAX) {
				BUBBLE_SCALE += BUBBLE_SCALE_STEP;
			}
			MUTEX_UNLOCK(lock_bubble_scale);
		}

		if (keys[S2I_KEY_DOWN]) {
			MUTEX_LOCK(lock_bubble_scale);
			if (BUBBLE_SCALE > BUBBLE_SCALE_MIN){
				BUBBLE_SCALE -= BUBBLE_SCALE_STEP;
			}
			MUTEX_UNLOCK(lock_bubble_scale);
		}

		if (keys[S2I_KEY_LEFT]) {
			MUTEX_LOCK(lock_bubble_tasks);
			if (bubble_tasks > BUBBLE_TASKS_MIN){
				bubble_tasks--;
			}
			MUTEX_UNLOCK(lock_bubble_tasks);
		}

		if (keys[S2I_KEY_RIGHT]) {
			MUTEX_LOCK(lock_bubble_tasks);
			if (bubble_tasks < BUBBLE_TASKS_MAX){
				bubble_tasks++;
			}
			MUTEX_UNLOCK(lock_bubble_tasks);
		}

		if (keys[S2I_KEY_PLUS]) {
			MUTEX_LOCK(lock_gain);
			if (GAIN < GAIN_MAX){
				GAIN += GAIN_STEP;
				update_gain_unsafe();
			}
			MUTEX_UNLOCK(lock_gain);
		}

		if (keys[S2I_KEY_MINUS]) {
			MUTEX_LOCK(lock_gain);
			if (GAIN > GAIN_MIN){
				GAIN -= GAIN_STEP;
				update_gain_unsafe();
			}
			MUTEX_UNLOCK(lock_gain);
		}

		if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
			keycode = event.keyboard.keycode;
			switch (keycode) {
				case ALLEGRO_KEY_ESCAPE:
					keys[S2I_KEY_ESCAPE] = 1;
				break;

				case ALLEGRO_KEY_UP:
					keys[S2I_KEY_UP] = 1;
				break;

				case ALLEGRO_KEY_DOWN:
					keys[S2I_KEY_DOWN] = 1;
				break;

				case ALLEGRO_KEY_LEFT:
					keys[S2I_KEY_LEFT] = 1;
				break;

				case ALLEGRO_KEY_RIGHT:
					keys[S2I_KEY_RIGHT] = 1;
				break;

				case 66: // plus sign //TODO: do it in a proper way
					keys[S2I_KEY_PLUS] = 1;
				break;

				case 74: // minus sign //TODO: do it in a proper way 
					keys[S2I_KEY_MINUS] = 1;
				break;

				default:
					break;
			}
		}

		if (event.type == ALLEGRO_EVENT_KEY_UP) {
			keycode = event.keyboard.keycode;
			switch (keycode) {
				case ALLEGRO_KEY_ESCAPE:
					keys[S2I_KEY_ESCAPE] = 0;
				break;

				case ALLEGRO_KEY_UP:
					keys[S2I_KEY_UP] = 0;
				break;

				case ALLEGRO_KEY_DOWN:
					keys[S2I_KEY_DOWN] = 0;
				break;

				case ALLEGRO_KEY_LEFT:
					keys[S2I_KEY_LEFT] = 0;
				break;

				case ALLEGRO_KEY_RIGHT:
					keys[S2I_KEY_RIGHT] = 0;
				break;

				case 66: // plus sign //TODO: do it in a proper way
					keys[S2I_KEY_PLUS] = 0;
				break;

				case 74: // minus sign //TODO: do it in a proper way
					keys[S2I_KEY_MINUS] = 0;
				break;
			default:
				break;
			}
		}

		EXP_LOCK(done_local = DONE, lock_done);
	}

	return NULL;
}

int main(int argc, char * argv[])
{
	char audio_filename[FILENAME_SIZE];

	pthread_mutex_init(&lock_done, NULL);
	pthread_mutex_init(&lock_audio_time_ms, NULL);
	pthread_mutex_init(&lock_bubble_scale, NULL);
	pthread_mutex_init(&lock_bubble_tasks, NULL);

	pthread_cond_init(&cond_producer, NULL);
	pthread_cond_init(&cond_consumers, NULL);
	pthread_mutex_init(&lock_fft, NULL);

	DONE = 0;
	GAIN = 50;
	var_fft = 0;
	bubble_tasks = BUBBLE_TASKS_BASE;
	audio_time_ms = 0;
	memset(audio_filename, 0, FILENAME_SIZE);

	if (argc < 2) {
		printf("Please provide an audio filename.\n");
		exit(EXIT_FILENAME_NOT_FOUND);
	}
	strncpy(audio_filename, argv[1], FILENAME_SIZE);
	fft_audio_init(&audio, audio_filename, FRAGMENT_SIZE, fft_audio_hamming);

	allegro_init();
	al_set_target_bitmap(NULL);

	mixer = al_get_default_mixer();
	stream = al_create_audio_stream(FRAGMENT_COUNT,
									FRAGMENT_SIZE,
									FRAGMENT_SAMPLERATE,
									ALLEGRO_AUDIO_DEPTH_FLOAT32,
									allegro_channel_conf_with(audio.channels));
	al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_ONCE);
	update_gain_unsafe();
	al_check(al_attach_audio_stream_to_mixer(stream, mixer),
			 "al_attach_audio_stream_to_mixer()");
	fill_stream_fragment();

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
	ptask_create(task_input, 0, 0, 0);
	ptask_create(task_display,
				 TASK_DISPLAY_PERIOD,
				 TASK_DISPLAY_DEADLINE,
				 TASK_DISPLAY_PRIORITY);
	ptask_wait_tasks();

	// Free
	pthread_mutex_destroy(&lock_bubble_tasks);
	pthread_mutex_destroy(&lock_bubble_scale);
	pthread_mutex_destroy(&lock_audio_time_ms);
	pthread_mutex_destroy(&lock_done);

	pthread_cond_destroy(&cond_producer);
	pthread_cond_destroy(&cond_consumers);
	pthread_mutex_destroy(&lock_fft);

	fft_audio_free(&audio);
	al_drain_audio_stream(stream);
	allegro_free();

	return 0;
}
