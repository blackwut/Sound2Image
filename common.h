#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

/**
    Constants
*/
#define FILENAME_SIZE           128
#define BUBBLES_TEXT_SIZE       64
#define TIMER_TEXT_SIZE         16
#define GAIN_TEXT_SIZE          16

#define DISPLAY_W               960
#define DISPLAY_H               480
#define DISPLAY_AUDIO_X         0
#define DISPLAY_AUDIO_Y         64
#define DISPLAY_AUDIO_W         DISPLAY_W
#define DISPLAY_AUDIO_H         (DISPLAY_H - (DISPLAY_AUDIO_Y * 2))
#define FRAMERATE               (50)

// #define INPUT_QUEUE_TIMEOUT     5
// #define TASK_INPUT_PERIOD       50
// #define TASK_INPUT_DEADLINE     50
// #define TASK_INPUT_PRIORITY     (49)
// #define TASK_INPUT_QUEUE_TIME   (TASK_INPUT_PERIOD - INPUT_QUEUE_TIMEOUT)


#define TASK_DISPLAY_PERIOD     (1000 / FRAMERATE)
#define TASK_DISPLAY_DEADLINE   (1000 / FRAMERATE)
#define TASK_DISPLAY_PRIORITY   (49)


#define TASK_FFT_PERIOD         (1000 / FRAMERATE)
#define TASK_FFT_DEADLINE       (1000 / FRAMERATE)
#define TASK_FFT_PRIORITY       (99)

#define TASK_BUBBLE_PERIOD       (1000 / FRAMERATE)
#define TASK_BUBBLE_DEADLINE     (1000 / FRAMERATE)
#define TASK_BUBBLE_PRIORITY     (69)

#define DEBUG 1

#define EXIT_ALLEGRO_ERROR          1
#define EXIT_FILENAME_NOT_FOUND     2

#define EXIT_MALLOC                 1
#define EXIT_OPEN_SF_FILE           2
#define EXIT_CLOSE_SF_FILE          3
#define EXIT_BQUEUE_TIMEOUT_FORMAT  4


#define MUTEX_LOCK(lock) pthread_mutex_lock(&lock)
#define MUTEX_UNLOCK(lock) pthread_mutex_unlock(&lock)

#define EXP_LOCK(exp, lock) \
    MUTEX_LOCK(lock); \
    exp; \
    MUTEX_UNLOCK(lock)


#define PRINT_TO(file, fmt, ...)\
    fprintf(file, "%s:%d\t"fmt, __func__, __LINE__, __VA_ARGS__)

#define MLOG(fmt, ...) \
    do { PRINT_TO(stdout, fmt, __VA_ARGS__); } while (0)

#define DLOG(fmt, ...) \
    do { if (DEBUG) PRINT_TO(stderr, fmt, __VA_ARGS__); } while(0)


#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#endif
