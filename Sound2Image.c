#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sndfile.h>
#include "common.h"
#include "allegro_utils.h"
#include "ptask.h"
#include "bqueue.h"
#include "fft_audio.h"
#include "time_utils.h"


#define RED(r)      (((r) *  7) % 255)
#define GREEN(g)    (((g) * 11) % 255)
#define BLUE(b)     (((b) * 13) % 255)
#define ALPHA(a)    (((a) *  5) % 255)

#define FRAGMENT_COUNT 4
#define FRAGMENT_FREQ 44100
#define FRAGMENT_SAMPLES (size_t) (FRAGMENT_FREQ / (float)TASK_FFT_PERIOD)

#define AUDIO_CHANNELS ALLEGRO_CHANNEL_CONF_2
ALLEGRO_EVENT_QUEUE * streamQueue;
ALLEGRO_AUDIO_STREAM * stream;
float * data_play = NULL;
bool DONE = false;
int samplerate = -1;

size_t audio_playing_time_ms = 0;

#define DISSOLVENCE_EFFECT_RATE 0.2

#define BUBBLE_TASKS 32

#define BUBBLE_THICKNESS     4
#define BUBBLE_ALPHA_FILLED  200
#define BUBBLE_ALPHA         255

#define BUBBLE_SCALE_MIN    0.5
#define BUBBLE_SCALE_BASE   1.0
#define BUBBLE_SCALE_MAX    2.0
#define BUBBLE_SCALE_STEP   0.1

float BUBBLE_SCALE = BUBBLE_SCALE_BASE;

typedef struct {
    float x;
    float y;
    float radius;

    unsigned char red;
    unsigned char green;
    unsigned char blue;
} Bubble;

Bubble bubbles[BUBBLE_TASKS];

void draw_bubble(Bubble * b)
{
    const float radius  = b->radius * BUBBLE_SCALE;
    const float x       = b->x + radius;
    const float y       = b->y + radius;

    ALLEGRO_COLOR color_filled = al_map_rgba(b->red, 
                                             b->green,
                                             b->blue,
                                             BUBBLE_ALPHA_FILLED);
    ALLEGRO_COLOR color = al_map_rgba(b->red,
                                      b->green,
                                      b->blue,
                                      BUBBLE_ALPHA);

    al_draw_filled_circle(x, y, radius, color_filled);
    al_draw_circle(x, y, radius, color, BUBBLE_THICKNESS);
}

#define BUFFER_SIZE 4096

BQueue audioQueue[BUBBLE_TASKS];

void bqueue_check(bool test, const char * description)
{
    if (test == BQUEUE_SUCCESS) return;
    fprintf(stderr, "BQUEUE error in %s\n", description);
    exit(1);
}

void init_fft_audio(char infilename[])
{
    SNDFILE * infile = NULL;
    SF_INFO sfinfo;
    sf_count_t frames;
    int channels;

    float * data = NULL;
    float buffer[BUFFER_SIZE];
    int readcount = 0;
    sf_count_t framesToRead;

    memset(&sfinfo, 0, sizeof(sfinfo));

    if ((infile = sf_open(infilename, SFM_READ, &sfinfo)) == NULL) {
        DLOG("Not able to open input file %s.\n", infilename);
        exit(EXIT_OPEN_SF_FILE);
    }

    // sf_command(infile, SFC_SET_NORM_FLOAT, NULL, SF_FALSE);

    channels    = sfinfo.channels;
    frames      = sfinfo.frames;
    samplerate  = sfinfo.samplerate;

    data = malloc(frames * sizeof(float));
    data_play = malloc(frames * 2 * sizeof(float));
    check_malloc(data, "data");
    check_malloc(data_play, "data_play");

    size_t dataplayid = 0;
    framesToRead = BUFFER_SIZE;
    while ((readcount = sf_read_float(infile, buffer, framesToRead)) > 0) {
        for (int k = 0; k < readcount; ++k) {
            data_play[dataplayid++] = buffer[k];
        }
    }

    for (int k = 0; k < frames; ++k) {
        const float ss1 = data_play[k * 2];
        const float ss2 = data_play[k * 2 + 1];
        float s1 = ss1 * (float) 0x8000;
        float s2 = ss2 * (float) 0x8000;

        s1 = (s1 > 0x7FFF ? 0x7FFF : s1);
        s2 = (s2 > 0x7FFF ? 0x7FFF : s2);

        data[k] = (s1 + s2) / 2.f;
    }

    printf("frames: %lld\tsecs: %lld\n", frames, frames / samplerate);
    fft_audio_init(data, frames, samplerate, samplerate);
}

void fill_fragment(float * fragment)
{
    static size_t i = 0;

    const size_t s = FRAGMENT_SAMPLES * (AUDIO_CHANNELS == ALLEGRO_CHANNEL_CONF_1 ? 1 : 2);
    for (int j = 0; j < s; ++j, ++i) {
        fragment[j] = data_play[i];
    }
}

void * task_fft(void * arg)
{
    ALLEGRO_EVENT event;
    float * streamBuffer;
    fft_audio_block * audio_block;
    const size_t windowSize = (samplerate / 2) / BUBBLE_TASKS;
    al_set_mixer_playing(al_get_default_mixer(), true);

    const int id = ptask_id(arg);
    ptask_activate(id);

    while (!DONE) {

        al_wait_for_event_timed(streamQueue, &event, TIME_MSEC_TO_SEC(1));
         if (event.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT) {
            streamBuffer = al_get_audio_stream_fragment(stream);
            if (streamBuffer != NULL) {
                fill_fragment(streamBuffer);
                al_check(al_set_audio_stream_fragment(stream, streamBuffer), "al_set_audio_stream_fragment");
                // float delay = FRAGMENT_COUNT * FRAGMENT_SAMPLES / (float)FRAGMENT_FREQ * 1000;
                // printf("delay: %f ms\n", delay);
            }
        }

        audio_block = (fft_audio_block *)calloc(1, sizeof(fft_audio_block));
        check_malloc(audio_block, "audio_block");

        int ret = fft_audio_block_shift_ms(audio_block, TASK_FFT_PERIOD);
        if (ret != FFT_AUDIO_SUCCESS) {
            DLOG("fft_audio_next_block exit with unsuccess!\n", NULL);
            break;
        }

        audio_playing_time_ms += TASK_FFT_PERIOD;

        for (int i = 0; i < BUBBLE_TASKS; ++i) {
            const size_t windowStart = i * windowSize;
            fft_audio_block * sub_block = (fft_audio_block *)calloc(1, sizeof(fft_audio_block));
            check_malloc(sub_block, "sub_block");

            fft_audio_get_sub_block(sub_block, audio_block, windowStart, windowSize);
            bqueue_enqueue(&(audioQueue[i]), sub_block);
        }

        free(audio_block);

        if (ptask_deadline_miss(id)) {
            DLOG("%d) deadline missed!\n", id);
        }
        ptask_wait_for_activation(id);
    }

    for (int i = 0; i < BUBBLE_TASKS; ++i) {
        bqueue_enqueue(&(audioQueue[i]), BQUEUE_EOF);
    }

    return NULL;
}

void * task_bubble(void * arg)
{
    const int id = ptask_id(arg);
    BQueue * inQueue = &(audioQueue[id]);
    fft_audio_block * audio_block;
    long power;
    long amplitude;
    long RMS;
    long dB;
    Bubble * bubble = &(bubbles[id]);

    ptask_activate(id);

    while (!DONE) {

        audio_block = (fft_audio_block *)bqueue_dequeue(inQueue, 0);
        if (audio_block == BQUEUE_EOF) {
            break;
        }

        if (audio_block == NULL) {
            printf("(%d) audio_block == NULL\n", id);
            break;
        }

        power     = (long)audio_block->stats.power;
        amplitude = (long)audio_block->stats.amplitude;
        dB        = (long)audio_block->stats.dB;
        RMS       = (long)audio_block->stats.RMS;


        bubble->radius   = dB / 4;
        bubble->x        = (id * (DISPLAY_W / BUBBLE_TASKS));
        // bubble->y        = DISPLAY_H - (amplitude % DISPLAY_H);
        bubble->y        = (DISPLAY_H - (dB / 90.f * DISPLAY_H));

        bubble->red      = RED(power);
        bubble->green    = GREEN(amplitude);
        bubble->blue     = BLUE(dB);

        free(audio_block);

        if (ptask_deadline_miss(id)) {
            DLOG("%d) deadline missed!\n", id);
        }
        ptask_wait_for_activation(id);
    }

    return NULL;
}


void * task_display(void * arg)
{
    char time_text[32] = {0};
    al_set_target_backbuffer(display);
    al_clear_to_color(background_color);
    al_flip_display();

    const int id = ptask_id(arg);
    ptask_activate(id);

    while (!DONE) {

        al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
        for (size_t i = 0; i < BUBBLE_TASKS; ++i) {
            draw_bubble(bubbles + i);
        }

        sprintf(time_text, "%04.2f s", audio_playing_time_ms / 1000.f);
        allegro_print_text(time_text, 24, 24);

        // Blur Effect
        al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
        al_draw_filled_rectangle(0, 0, DISPLAY_W, DISPLAY_H, al_map_rgba_f(0, 0, 0, DISSOLVENCE_EFFECT_RATE));
        al_flip_display();

        if (ptask_deadline_miss(id)) {
            DLOG("(%d) deadline missed!\n", id, NULL);
        }
        ptask_wait_for_activation(id);
    }

    allegro_free();

    return NULL;
}

void * task_input(void * arg)
{
    ALLEGRO_EVENT event;
    int keycode;

    const int id = ptask_id(arg);
    ptask_activate(id);

    while (true) {
        memset(&event, 0, sizeof(ALLEGRO_EVENT));
        al_wait_for_event_timed(queue, &event, TIME_MSEC_TO_SEC(TASK_INPUT_QUEUE_TIME));
        
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

                // if (keycode == ALLEGRO_KEY_ENTER) {
                //     allegro_screenshot("/Volumes/RamDisk/", "test");
                // }

                break;
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                DONE = true;
                break;
            default:
                break;
        }

        if(DONE) break;

        if (ptask_deadline_miss(id)) {
            DLOG("(%d) deadline missed!\n", id, NULL);
        }
        ptask_wait_for_activation(id);
    }

    return NULL;
}



// void * task_play_sound(void * args)
// {
//     float * buf;
//     while (!DONE) {
//         ALLEGRO_EVENT event;
//         al_wait_for_event_timed(streamQueue, &event, TIME_MSEC_TO_SEC(5));

//         if (event.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT) {
//             buf = al_get_audio_stream_fragment(stream);
//             if (!buf) {
//                 continue;
//             }

//             fill_fragment(buf);

//             al_check(al_set_audio_stream_fragment(stream, buf), "al_set_audio_stream_fragment");
//             // float delay = FRAGMENT_COUNT * FRAGMENT_COUNT / (float)FRAGMENT_FREQ * 1000;
//             // printf("delay: %f ms\n", delay);
//         }
//     }

//    al_drain_audio_stream(stream);
//    al_destroy_event_queue(queue);

//    return NULL;
// }

int main(int argc, char * argv[])
{
    //TODO: ELIMINARE TUTTE LE CREAZIONI DI MEMORIA CON LE MALLOC/ CALLOC
    //TODO: USARE LA FORMATTAZZIONE DEL PROF OVUNQUE
    //TODO: TUTTO GLOBALE (??)
    //TODO: SENTIRE L'AUDIO MENTRE SI VISUALIZZA L'IMMAGINE DINAMICA
    //TODO: CARATTERIZZARE GLI AUDIO (SE è UNA CANZONE RITMICA, ROCK, LENTA, ECC)
    //TODO: FILTRO PASSA BASSO
    //TODO: POSSIBILITA' DI AUMENTARE E DIMINUIRE IL NUMERO DI BUBBLES (THREADS)
    //TODO: POSSIBILITA' DI CAMBIARE I COLORI
    //TODO: MIGLIORARE L'EFFETTO VISIVO
    allegro_init();
    al_set_target_bitmap(NULL);

    // Audio
    char audioName[1024] = "wav/test.wav";
    if (argc > 1) {
        sprintf(audioName, "%s", argv[1]);
    }

    printf("audioName: %s\n", audioName);
    init_fft_audio(audioName);

    stream = al_create_audio_stream(FRAGMENT_COUNT,
                                    FRAGMENT_SAMPLES,            // samples
                                    FRAGMENT_FREQ,               // freq
                                    ALLEGRO_AUDIO_DEPTH_FLOAT32, // depth
                                    AUDIO_CHANNELS);             // chan_conf

    ALLEGRO_MIXER * mixer = al_get_default_mixer();
    if (!al_attach_audio_stream_to_mixer(stream, mixer)) {
        printf("Could not attach stream to mixer.\n");
        return 1;
    }
    
    al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_ONCE);
    streamQueue = al_create_event_queue();
    al_register_event_source(streamQueue, al_get_audio_stream_event_source(stream));
    al_set_mixer_playing(mixer, false);
    // ptask_create(task_play_sound, 100, 100, TASK_FFT_PRIORITY);

    // Queues
    for (int i = 0; i < BUBBLE_TASKS; ++i) {
        bqueue_check(bqueue_create(audioQueue + i), "bqueue_create");
    }

    // Tasks
    for (int i = 0; i < BUBBLE_TASKS; ++i) {
        ptask_create(task_bubble, TASK_BUBBLE_PERIOD, TASK_BUBBLE_DEADLINE, TASK_BUBBLE_PRIORITY);
    }

    ptask_create(task_fft,  TASK_FFT_PERIOD, TASK_FFT_DEADLINE , TASK_FFT_PRIORITY);
    ptask_create(task_display, TASK_DISPLAY_PERIOD, TASK_DISPLAY_DEADLINE, TASK_DISPLAY_PRIORITY);
    ptask_create(task_input, TASK_INPUT_PERIOD, TASK_INPUT_DEADLINE, TASK_INPUT_PRIORITY);

    ptask_wait_tasks();

    // Free
    fft_audio_free();
    for (int i = 0; i < BUBBLE_TASKS; ++i) {
        bqueue_destroy(audioQueue + i, free);
    }

    al_drain_audio_stream(stream);
    al_destroy_event_queue(streamQueue);
    allegro_free();

    return 0;
}
