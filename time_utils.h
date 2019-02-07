#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <time.h>

#define TIME_MILLISEC   (1000)
#define TIME_MICROSEC   (1000000)
#define TIME_NANOSEC    (1000000000)

#define TIME_SEC_TO_MSEC(t)     ((t) * TIME_MILLISEC)
#define TIME_SEC_TO_NSEC(t)     ((t) * TIME_NANOSEC)
#define TIME_MSEC_TO_SEC(t)     ((t) / (float)TIME_MILLISEC)
#define TIME_MSEC_TO_NSEC(t)    ((t) * TIME_MICROSEC)
#define TIME_NSEC_TO_SEC(t)     ((t) / (float)TIME_NANOSEC)
#define TIME_NSEC_TO_MSEC(t)    ((t) / (float)TIME_MICROSEC)


static inline int time_now(struct timespec * t)
{
    return clock_gettime(CLOCK_MONOTONIC, t);
}

static inline void time_copy(struct timespec * td,
                             const struct timespec ts)
{
    td->tv_sec  = ts.tv_sec;
    td->tv_nsec = ts.tv_nsec;
}

static inline int time_cmp(const struct timespec t1,
                           const struct timespec t2)
{
    if (t1.tv_sec > t2.tv_sec) return 1;
    if (t1.tv_sec < t2.tv_sec) return -1;
    if (t1.tv_nsec > t2.tv_nsec) return 1;
    if (t1.tv_nsec < t2.tv_nsec) return -1;
    return 0;
}

static inline void time_add_ms(struct timespec * td,
                               const int ms)
{
    td->tv_sec += TIME_MSEC_TO_SEC(ms);
    td->tv_nsec += (ms % TIME_MILLISEC) * TIME_MICROSEC;
    if (td->tv_nsec > TIME_NANOSEC) {
        td->tv_nsec -= TIME_NANOSEC;
        td->tv_sec += 1;
    }
}

static inline void time_diff(struct timespec * td,
                             const struct timespec * ts)
{
    td->tv_sec = ts->tv_sec - td->tv_sec;
    td->tv_nsec = ts->tv_nsec - td->tv_nsec;
    if (td->tv_sec < 0) {
        td->tv_sec = 0;
        td->tv_nsec = 0;
    } else if (td->tv_nsec < 0) {
        if (td->tv_sec == 0) {
            td->tv_sec = 0;
            td->tv_nsec = 0;
        } else {
            td->tv_sec = td->tv_sec - 1;
            td->tv_nsec = td->tv_nsec + TIME_NANOSEC;
        }
    }
}

#ifdef __MACH__
/* emulate clock_nanosleep for CLOCK_MONOTONIC and TIMER_ABSTIME */
static inline int time_nanosleep(const struct timespec * req)
{
    struct timespec diff;
    int retval = time_now(&diff);
    if (retval == 0) {
        time_diff(&diff, req);
        retval = nanosleep(&diff, NULL);
    }
    return retval;
}
#else /* POSIX */
    /* clock_nanosleep for CLOCK_MONOTONIC and TIMER_ABSTIME */
    #define time_nanosleep(req) \
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, (req), NULL)
#endif

static inline void time_print_ms(const struct timespec * t, const char * message)
{
    size_t ns = TIME_SEC_TO_NSEC(t->tv_sec) + t->tv_nsec;
    printf("%s: \t%zu\n", message, ns);
}

#endif
