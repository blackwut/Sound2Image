#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

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
