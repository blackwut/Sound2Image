#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

/**
    Constants
*/
#define QUEUE_TIMEOUT           (5)

#define DISPLAY_W               (800)
#define DISPLAY_H               (600)
#define FRAMERATE               (30)

#define TASK_INPUT_PERIOD       (1000 / FRAMERATE)
#define TASK_INPUT_DEADLINE     (1000 / FRAMERATE)
#define TASK_INPUT_PRIORITY     (99)
#define TASK_INPUT_QUEUE_TIME   (TASK_DISPLAY_PERIOD - QUEUE_TIMEOUT)


#define TASK_DISPLAY_PERIOD     (1000 / FRAMERATE)
#define TASK_DISPLAY_DEADLINE   (1000 / FRAMERATE)
#define TASK_DISPLAY_PRIORITY   (99)


#define TASK_FFT_PERIOD         (1000 / FRAMERATE)
#define TASK_FFT_DEADLINE       (1000 / FRAMERATE)
#define TASK_FFT_PRIORITY       (49)

#define TASK_BUBBLE_PERIOD       (1000 / FRAMERATE)
#define TASK_BUBBLE_DEADLINE     (1000 / FRAMERATE)
#define TASK_BUBBLE_PRIORITY     (49)

#define DEBUG 1

#define EXIT_MALLOC                 1
#define EXIT_OPEN_SF_FILE           2
#define EXIT_CLOSE_SF_FILE          3
#define EXIT_BQUEUE_TIMEOUT_FORMAT  4


#define PRINT_TO(file, fmt, ...)\
    fprintf(file, "%s:%d\t"fmt, __func__, __LINE__, __VA_ARGS__)

#define MLOG(fmt, ...) \
    do { PRINT_TO(stdout, fmt, __VA_ARGS__); } while (0)

#define DLOG(fmt, ...) \
    do { if (DEBUG) PRINT_TO(stderr, fmt, __VA_ARGS__); } while(0)


#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static inline void check_malloc(const void * p, const char message[])
{
    if (p != NULL) return;
    DLOG("ERROR: malloc \"%s\"\n", message);
    exit(EXIT_MALLOC);
}

#endif
