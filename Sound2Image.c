#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sndfile.h>
#include "common.h"
#include "allegro_utils.h"
#include "utils.h"
#include "ptask.h"
#include "bqueue.h"
#include "fft_audio.h"


#define SHAPE_TASKS 7
#define BUFFER_SIZE 4096


bool done = false;

int samplerate = -1;

BQueue audioQueue[SHAPE_TASKS];
BQueue shapeQueue;


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

    fft_audio_init(data, frames, samplerate, 4 * samplerate);
}

void * task_audio(void * arg)
{
    fft_audio_block * audio_block;
    const size_t windowSize = samplerate / SHAPE_TASKS;


    const int id = ptask_id(arg);
    ptask_activate(id);

    while (!done) {

        audio_block = (fft_audio_block *)calloc(1, sizeof(fft_audio_block));
        check_malloc(audio_block, "audio_block");

        if (fft_audio_next_block(audio_block) != FFT_AUDIO_SUCCESS) {
            DLOG("fft_audio_next_block exit with unsuccess!\n", NULL);
            // done = true;
            break;
        }

        for (int i = 0; i < SHAPE_TASKS; ++i) {
            const size_t windowStart = i * windowSize;
            fft_audio_block * sub_block = (fft_audio_block *)calloc(1, sizeof(fft_audio_block));
            check_malloc(sub_block, "sub_block");

            // DLOG("id: %d\tstart: %zu\tsize: %zu\n", i, windowStart, windowSize);

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
    for (int i = 0; i < SHAPE_TASKS; ++i) {
        bqueue_enqueue(&(audioQueue[i]), BQUEUE_EOF);
    }

    return NULL;
}

void * task_shape(void * arg)
{
    fft_audio_block * audio_block;
    BQueue * inQueue;


    const int id = ptask_id(arg);
    ptask_activate(id);

    inQueue = &(audioQueue[id]);

    while (!done) {
        audio_block = (fft_audio_block *)bqueue_dequeue(inQueue, 0);

        if (audio_block == BQUEUE_EOF) {
            bqueue_enqueue(&shapeQueue, BQUEUE_EOF);
            break;
        }

        // Shape_bmp * shape = shape_bmp_line(&(audio_block->stats));

        // //TODO: use bitmap and use phase to rotate the shape
        Shape * shape = (Shape *)calloc(1, sizeof(Shape));

        switch (id) {
            case LINE:
                shape_line(shape, &(audio_block->stats));
                break;

            case TRIANGLE:
                shape_triangle(shape, &(audio_block->stats));
                break;

            case RECTANGLE:
                shape_rectangle(shape, &(audio_block->stats));
                break;

            case ELLIPSE:
                shape_ellipse(shape, &(audio_block->stats));
                break;

            case TRIANGLE_FILLED:
                shape_filled_triangle(shape, &(audio_block->stats));
                break;

            case RECTANGLE_FILLED:
                shape_filled_rectangle(shape, &(audio_block->stats));
                break;

            case ELLIPSE_FILLED:
                shape_filled_ellipse(shape, &(audio_block->stats));
                break;

            default:
                printf("Huston we have a problem!\n");
                break;
        }

        bqueue_enqueue(&shapeQueue, shape);
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
    allegro_init();

    int tasks_exited = 0;

    const int id = ptask_id(arg);
    ptask_activate(id);

    while (true) {
        
        ALLEGRO_EVENT event;
        al_wait_for_event_timed(queue, &event, 0.005);
        
        switch(event.type) {
            case ALLEGRO_EVENT_KEY_DOWN:
                if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                    done = true;
                break;

            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                done = true;
                break;

            default:
                break;
        }

        if(done) break;

        // Shape_bmp * shape = bqueue_dequeue(&shapeQueue, 4);
        Shape * shape = bqueue_dequeue(&shapeQueue, 4);

        if (shape != NULL) {
            if (shape == BQUEUE_EOF) {
            tasks_exited += 1;
                if (tasks_exited == SHAPE_TASKS) {
                    done = true;
                    break;
                }
            } else {
                // al_draw_scaled_rotated_bitmap(shape->bmp,
                //                               shape->cx,
                //                               shape->cy,
                //                               shape->dx,
                //                               shape->dy,
                //                               shape->xscale,
                //                               shape->yscale,
                //                               shape->angle,
                //                               0);
                draw(shape);
                // al_destroy_bitmap(shape->bmp);
                free(shape);
            }
        }

        al_flip_display();

        if (ptask_deadline_miss(id)) {
            DLOG("(%d) deadline missed!\n", id, NULL);
        }
        ptask_wait_for_activation(id);
    }

    allegro_screenshot("/Volumes/RamDisk/", "test");
    allegro_free();

    return NULL;
}

int main(int argc, char * argv[])
{
    // Audio
    init_fft_audio("wav/test2.wav");

    // Queues
    bqueue_check(bqueue_create(&shapeQueue), "bqueue_create");

    for (int i = 0; i < SHAPE_TASKS; ++i) {
        bqueue_check(bqueue_create(audioQueue + i), "bqueue_create");
    }

    // Tasks
    for (int i = 0; i < SHAPE_TASKS; ++i) {
        ptask_create(task_shape, 100, 100, 10);
    }

    ptask_create(task_audio,  100, 100, 10);
    ptask_create(task_display, 30,  30, 10);

    ptask_wait_tasks();

    // Free
    fft_audio_free();
    bqueue_destroy(&shapeQueue, free);
    for (int i = 0; i < SHAPE_TASKS; ++i) {
        bqueue_destroy(audioQueue + i, free);
    }

    return 0;
}
