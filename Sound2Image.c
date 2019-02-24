#include <stdio.h>
#include <stdlib.h> 
#include <math.h>
#include "common.h"
#include "fft_audio.h"
#include "allegro_utils.h"
#include "ptask.h"
#include "time_utils.h"

#define BUBBLES_INFO_TEXT_ALIGN ALLEGRO_ALIGN_LEFT
#define BUBBLES_INFO_X 10
#define BUBBLES_INFO_Y 0

#define TIMER_INFO_TEXT_ALIGN ALLEGRO_ALIGN_RIGHT
#define TIMER_INFO_X DISPLAY_W - 10
#define TIMER_INFO_Y 0

#define USER_INFO_TEXT	"UP: bigger bubbles - DOWN: smaller bubbles - " \
						"LEFT: less bubbles - RIGHT: more bubbles"
#define USER_INFO_TEXT_ALIGN ALLEGRO_ALIGN_CENTER
#define USER_INFO_X DISPLAY_W / 2
#define USER_INFO_Y DISPLAY_H - 32


#define RED(r)		(((long)(r) * 1) % 255)
#define GREEN(g)	(((long)(g) * 1) % 255)
#define BLUE(b)		(((long)(b) * 1) % 255)
#define ALPHA(a)	(((long)(a) * 1) % 255)

#define FRAGMENT_SAMPLERATE		44100
#define FRAGMENT_COUNT			50
#define FRAGMENT_SIZE			882 // Must be a diveder of both FRAGMENT_SAMPLERATE and FRAMERATE
#define FRAGMENT_DEPTH 			ALLEGRO_AUDIO_DEPTH_FLOAT32

#define BUBBLE_TASKS_MIN 8
#define BUBBLE_TASKS_BASE 24
#define BUBBLE_TASKS_MAX 32
#define BUBBLE_FILTER_PARAM 0.8


#define DISSOLVENCE_EFFECT_RATE 0.2

#define BUBBLE_THICKNESS     2
#define BUBBLE_ALPHA_FILLED  200
#define BUBBLE_ALPHA         255

#define BUBBLE_SCALE_MIN    0.5f
#define BUBBLE_SCALE_BASE   1.0f
#define BUBBLE_SCALE_MAX    2.0f
#define BUBBLE_SCALE_STEP   0.1f


unsigned char colors[32][3] = {
//Gray
{ 46,  64,  83},
{ 52,  73,  94},
{ 86, 101, 115},
{ 93, 109, 126},
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

// unsigned char colors[32][3] = {
// {169,  50,  38},
// {192,  57,  43},
// {203,  67,  53},
// {231,  76,  60},
// {136,  78, 160},
// {155,  89, 182},
// {125,  60, 152},
// {142,  68, 173},
// { 36, 113, 163},
// { 41, 128, 185},
// { 46, 134, 193},
// { 52, 152, 219},
// { 23, 165, 137},
// { 26, 188, 156},
// { 19, 141, 117},
// { 22, 160, 133},
// { 34, 153,  84},
// { 39, 174,  96},
// { 40, 180,  99},
// { 46, 204, 113},
// {212, 172,  13},
// {241, 196,  15},
// {214, 137,  16},
// {243, 156,  18},
// {202, 111,  30},
// {230, 126,  34},
// {186,  74,   0},
// {211,  84,   0},
// {112, 123, 124},
// {127, 140, 141},
// { 46,  64,  83},
// { 52,  73,  94}
// };

float BUBBLE_SCALE = BUBBLE_SCALE_BASE; //TODO: LOCK

typedef struct {
	float x;
	float y;
	float radius;

	unsigned char red;
	unsigned char green;
	unsigned char blue;

	size_t freq;
} Bubble;

Bubble bubbles[BUBBLE_TASKS_MAX]; //TODO: LOCK

size_t bubble_tasks; //TODO: LOCK

bool DONE; //TODO: LOCK
size_t audio_time_ms; //TODO: LOCK
fft_audio audio; //TODO: LOCK
fft_audio_stats audio_stats[BUBBLE_TASKS_MAX];

ALLEGRO_AUDIO_STREAM * stream;
ALLEGRO_MIXER * mixer;

void draw_bubble(Bubble * b)
{
	const float radius		= b->radius * BUBBLE_SCALE;
	const float x			= (radius == 0 ? -radius : b->x);
	const float y			= (radius == 0 ? -radius : b->y);
	const float thickness	= BUBBLE_THICKNESS * BUBBLE_SCALE;
	char buffer[8];

	ALLEGRO_COLOR color_filled = al_map_rgba(b->red, 
											b->green,
											b->blue,
											BUBBLE_ALPHA_FILLED);
	ALLEGRO_COLOR color = al_map_rgba(b->red,
									b->green,
									b->blue,
									BUBBLE_ALPHA);

	snprintf(buffer, 8, "%zu", b->freq);

	al_draw_filled_circle(x, y, radius, color_filled);
	al_draw_circle(x, y, radius, color, thickness);
	allegro_print_text_small_color(buffer, color, x, DISPLAY_AUDIO_H + DISPLAY_AUDIO_Y + 20 , ALLEGRO_ALIGN_CENTER);
}

void try_fill_stream_fragment()
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

void * task_fft(void * arg)
{
	const int id = ptask_id(arg);
	int ret;
	ptask_activate(id);

	while (!DONE) {
		try_fill_stream_fragment();

		ret = fft_audio_next_window(&audio);
		if (ret == FFT_AUDIO_EOF) {
			DONE = true;
		}

		audio_time_ms += TASK_FFT_PERIOD;

		if (ptask_deadline_miss(id)) {
			DLOG("%d) deadline missed!\n", id);
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}

void * task_bubble(void * arg)
{
	const int id = ptask_id(arg);
	size_t i;
	size_t window_elements;
	float bubble_spacing;
	size_t from_sample = 0;
	size_t to_sample = 0;
	float y = 0.0f;

	ptask_activate(id);

	while (!DONE) {

		bubble_spacing	= ((float)DISPLAY_W / (float)(bubble_tasks + 1));
		window_elements	= FRAGMENT_SAMPLERATE / 2 / bubble_tasks; // TODO: avoid to read audio

		if (id < bubble_tasks) {

			from_sample = pow(2, (log2(FRAGMENT_SIZE / 2) - 3.0) / bubble_tasks * id + 3.0);
			to_sample = pow(2, (log2(FRAGMENT_SIZE / 2) - 3.0)/ bubble_tasks * (id + 1) + 3.0);

			fft_audio_stats_samples(&(audio_stats[id]),
									&audio,
									from_sample,
									to_sample);

			float dbMin = 10 * logf(audio.stats.magMin);
			float dbMax = 10 * logf(audio.stats.magMax);
			float dbVal = 10 * logf(audio_stats[id].magMax);

			if (isfinite(dbVal) && isfinite(dbMin) && isfinite(dbMax)){
				y = y * BUBBLE_FILTER_PARAM + (1.0 - BUBBLE_FILTER_PARAM) * (dbVal - dbMin) / (dbMax - dbMin);
			}

			bubbles[id].radius	= 8;
			bubbles[id].x		= (id + 1) * bubble_spacing;
			bubbles[id].y		= DISPLAY_AUDIO_Y + DISPLAY_AUDIO_H - y * DISPLAY_AUDIO_H;
			bubbles[id].red		= colors[id % 200][0];
			bubbles[id].green	= colors[id % 200][1];
			bubbles[id].blue	= colors[id % 200][2];
			bubbles[id].freq	= to_sample * FRAGMENT_SAMPLERATE / FRAGMENT_SIZE; //TODO: do something to avoid locks here

		} else {
			bubbles[id].radius = 0;
		}

		if (ptask_deadline_miss(id)) {
			DLOG("%d) deadline missed!\n", id);
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}


void * task_display(void * arg)
{
	const int id = ptask_id(arg);
	size_t i;
	size_t minutes;
	size_t seconds;
	size_t milliseconds;
	char bubbles_text[BUBBLES_TEXT_SIZE];
	char timer_text[TIMER_TEXT_SIZE];

	memset(timer_text, 0, TIMER_TEXT_SIZE);
	al_set_target_backbuffer(display);
	al_clear_to_color(BACKGROUND_COLOR);
	al_flip_display();

	ptask_activate(id);

	while (!DONE) {
		// Blur Effect
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
		al_draw_filled_rectangle(0, 0, DISPLAY_W, DISPLAY_H, al_map_rgba_f(16/255.f, 16/255.f, 16/255.f, DISSOLVENCE_EFFECT_RATE));

		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
		for (i = 0; i < bubble_tasks; ++i) {
			draw_bubble(&(bubbles[i]));
		}

		minutes = TIME_MSEC_TO_SEC(audio_time_ms) / 60;
		seconds = TIME_MSEC_TO_SEC(audio_time_ms) % 60;
		milliseconds = audio_time_ms % TIME_MILLISEC;
		snprintf(bubbles_text, BUBBLES_TEXT_SIZE, "Bubbles: %zu", bubble_tasks);
		snprintf(timer_text, TIMER_TEXT_SIZE, "%02zu:%02zu:%03zu", minutes, seconds, milliseconds);
		allegro_print_text(bubbles_text,
						   BUBBLES_INFO_X, BUBBLES_INFO_Y,
						   BUBBLES_INFO_TEXT_ALIGN);
		allegro_print_text(timer_text,
						   TIMER_INFO_X, TIMER_INFO_Y,
						   TIMER_INFO_TEXT_ALIGN);
		allegro_print_text(USER_INFO_TEXT,
						   USER_INFO_X, USER_INFO_Y,
						   USER_INFO_TEXT_ALIGN);
		al_flip_display();

		if (ptask_deadline_miss(id)) {
			DLOG("(%d) deadline missed!\n", id, NULL);
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}

void * task_input(void * arg)
{
	const int id = ptask_id(arg);
	ALLEGRO_EVENT event;
	int keycode;

	ptask_activate(id);

	while (!DONE) {
		memset(&event, 0, sizeof(ALLEGRO_EVENT));
		//TODO: verify TIME_MSEC_TO_SEC is still a float
		al_wait_for_event_timed(queue, &event,
								TIME_MSEC_TO_SEC((float)TASK_INPUT_QUEUE_TIME));
		
		switch(event.type) {
			case ALLEGRO_EVENT_KEY_DOWN:
				keycode = event.keyboard.keycode;
				
				if (keycode == ALLEGRO_KEY_ESCAPE) {
					DONE = true;
				}

				if (keycode == ALLEGRO_KEY_UP) {
					if (BUBBLE_SCALE < BUBBLE_SCALE_MAX) {
						BUBBLE_SCALE += BUBBLE_SCALE_STEP;
					}
				}

				if (keycode == ALLEGRO_KEY_DOWN) {
					if (BUBBLE_SCALE > BUBBLE_SCALE_MIN){
						BUBBLE_SCALE -= BUBBLE_SCALE_STEP;
					}
				}

				if (keycode == ALLEGRO_KEY_LEFT) {
					if (bubble_tasks > BUBBLE_TASKS_MIN){
						bubble_tasks--;
					}
				}

				if (keycode == ALLEGRO_KEY_RIGHT) {
					if (bubble_tasks < BUBBLE_TASKS_MAX){
						bubble_tasks++;
					}
				}
				break;

			case ALLEGRO_EVENT_DISPLAY_CLOSE:
				DONE = true;
				break;

			default:
				break;
		}

		if (ptask_deadline_miss(id)) {
			DLOG("(%d) deadline missed!\n", id, NULL);
		}
		ptask_wait_for_activation(id);
	}

	return NULL;
}

void prepare_audio_stream()
{
	size_t i;
	size_t fragments;
	int ret;

	fragments = al_get_available_audio_stream_fragments(stream);
	for (i = 0; i < FRAGMENT_COUNT / 2; ++i) { //TODO: verify why sometimes try_fill_stream_fragment fails
		try_fill_stream_fragment();

		ret = fft_audio_next_window(&audio);
		if (ret == FFT_AUDIO_EOF) {
			DONE = true;
		}
	}
}

int main(int argc, char * argv[])
{
	char audio_filename[FILENAME_SIZE];

	DONE = false;
	bubble_tasks = BUBBLE_TASKS_BASE;
	audio_time_ms = 0;
	memset(audio_filename, 0, FILENAME_SIZE);

	if (argc < 2) {
		printf("Please provide an audio filename.\n");
		exit(EXIT_FILENAME_NOT_FOUND);
	}
	strncpy(audio_filename, argv[1], FILENAME_SIZE);
	fft_audio_init(&audio, audio_filename, FRAGMENT_SIZE);

	allegro_init();
	al_set_target_bitmap(NULL);

	mixer = al_get_default_mixer();
	// al_set_mixer_playing(mixer, false);
	stream = al_create_audio_stream(FRAGMENT_COUNT,
									FRAGMENT_SIZE,
									FRAGMENT_SAMPLERATE,
									ALLEGRO_AUDIO_DEPTH_FLOAT32,
									allegro_channel_conf_with(audio.channels));
	al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_ONCE);
	al_check(al_attach_audio_stream_to_mixer(stream, mixer),
			 "al_attach_audio_stream_to_mixer()");
	prepare_audio_stream();

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
	fft_audio_free(&audio);
	al_drain_audio_stream(stream);
	allegro_free();

	return 0;
}
