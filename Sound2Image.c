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


/*
Slow dub/reggae: 75 BPM
Fast dub/reggae: 90 BPM
Deep house: 96 BPM
Slow house: 120 BPM
Electro house: 130 BPM
Trance: 135 BPM
Dubstep: 140 BPM
Hard house: 145 BPM
Jungle: 160 BPM
Drum&Bass: 175 BPM
Gabber: 190 BPM
*/

#define RED(r)      (((r) *  7) % 255)
#define GREEN(g)    (((g) * 11) % 255)
#define BLUE(b)     (((b) * 13) % 255)
#define ALPHA(a)    (((a) * 5) % 255)

bool DONE = false;
int samplerate = -1;

#define DISSOLVENCE_EFFECT_RATE 0.1

#define BUBBLE_TASKS 4

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

void draw_bubble(const Bubble * b)
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
BQueue shapeQueue;
BQueue bubbleQueue;

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
    int dataIndex = 0;
    float buffer[BUFFER_SIZE];
    int readcount = 0;
    sf_count_t framesToRead;

    memset(&sfinfo, 0, sizeof(sfinfo));

    if ((infile = sf_open(infilename, SFM_READ, &sfinfo)) == NULL) {
        DLOG("Not able to open input file %s.\n", infilename);
        exit(EXIT_OPEN_SF_FILE);
    }

    sf_command(infile, SFC_SET_NORM_FLOAT, NULL, SF_FALSE);

    channels    = sfinfo.channels;
    frames      = sfinfo.frames;
    samplerate  = sfinfo.samplerate;

    data = malloc(frames * sizeof(float));
    check_malloc(data, "data");

    framesToRead = BUFFER_SIZE / channels;
    while ((readcount = sf_readf_float(infile, buffer, framesToRead)) > 0) {
        for (int k = 0 ; k < readcount ; k++) {
            float sum = 0.f;
            for (int m = 0 ; m < channels ; m++) {
                sum += buffer[k * channels + m];
            }
            data[dataIndex++] = sum / channels;
        }
    }

    if (sf_close(infile) != 0) {
        DLOG("Not able to close input file %s.\n", infilename);
        exit(EXIT_CLOSE_SF_FILE);
    }

    fft_audio_init(data, frames, samplerate, 1 * samplerate);
}

void * task_fft(void * arg)
{
    fft_audio_block * audio_block;
    const size_t windowSize = samplerate / BUBBLE_TASKS;
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

    const int id = ptask_id(arg);
    ptask_activate(id);

    while (!DONE) {

        audio_block = (fft_audio_block *)calloc(1, sizeof(fft_audio_block));
        check_malloc(audio_block, "audio_block");

        int ret = fft_audio_next_block(audio_block);
        if (ret != FFT_AUDIO_SUCCESS) {
            DLOG("fft_audio_next_block exit with unsuccess!\n", NULL);
            break;
        }

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

    DLOG("%d) exit\n", id);
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

    ptask_activate(id);

    while (!DONE) {
        audio_block = (fft_audio_block *)bqueue_dequeue(inQueue, 0);

        if (audio_block == BQUEUE_EOF) {
            bqueue_enqueue(&shapeQueue, BQUEUE_EOF);
            break;
        }

        Bubble * bubble = (Bubble *)calloc(1, sizeof(Bubble));

        power     = (long)audio_block->stats.power;
        amplitude = (long)audio_block->stats.amplitude;
        dB        = (long)audio_block->stats.dB;
        RMS       = (long)audio_block->stats.RMS;

        bubble->radius  = dB / 2;
        bubble->x       = (power % DISPLAY_W);
        bubble->y       = (amplitude % DISPLAY_H);
        bubble->red     = RED(power);
        bubble->green   = GREEN(amplitude);
        bubble->blue    = BLUE(dB);

        bqueue_enqueue(&bubbleQueue, bubble);
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
    al_set_target_backbuffer(display);
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
    al_clear_to_color(al_map_rgba(0, 0, 0, 0));
    al_flip_display();

    int tasks_exited = 0;

    const int id = ptask_id(arg);
    ptask_activate(id);

    while (!DONE) {

        for (size_t indice = 0; indice < BUBBLE_TASKS; ++indice) {
            ALLEGRO_BITMAP * bubble = bqueue_dequeue(&bubbleQueue, 5);
            if (bubble != NULL) {
                if (bubble == BQUEUE_EOF) {
                    tasks_exited += 1;
                    if (tasks_exited == BUBBLE_TASKS) {
                        DONE = true;
                        break;
                    }
                } else {
                    draw_bubble(bubble);
                    // allegro_draw_bubble(x, y, 50);
                    // al_draw_bitmap(bubble, x + 20, y + 20, 0);
                    free(bubble);
                }
            }
        }

        // Blur Effect
        al_draw_filled_rectangle(0, 0, DISPLAY_W, DISPLAY_H, al_map_rgba_f(0.0, 0.0, 0.0, DISSOLVENCE_EFFECT_RATE));
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

int main(int argc, char * argv[])
{
    //TODO: ELIMINARE TUTTE LE CREAZIONI DI MEMORIA CON LE MALLOC/ CALLOC
    //TODO: USARE LA FORMATTAZZIONE DEL PROF OVUNQUE
    //TODO: TUTTO GLOBALE (??)
    //TODO: SENTIRE L'AUDIO MENTRE SI VISUALIZZA L'IMMAGINE DINAMICA
    //TODO: CARATTERIZZARE GLI AUDIO (SE è UNA CANZONE RITMICA, ROCK, LENTA, ECC)
    //TODO: INTERAZIONE CON L'UTENTE
    allegro_init();
    al_set_target_bitmap(NULL);

    // Audio
    char audioName[1024] = {0};
    if (argc > 1) {
        sprintf(audioName, "%s", argv[1]);
    } else {
        sprintf(audioName, "wav/test.wav");
    }

    printf("audioName: %s\n", audioName);
    init_fft_audio(audioName);

    // Queues
    bqueue_check(bqueue_create(&shapeQueue), "bqueue_create");
    bqueue_check(bqueue_create(&bubbleQueue), "bubbleQueue");

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
    bqueue_destroy(&shapeQueue, free);
    for (int i = 0; i < BUBBLE_TASKS; ++i) {
        bqueue_destroy(audioQueue + i, free);
    }

    return 0;
}
