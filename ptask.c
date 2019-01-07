#include "ptask.h"
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <assert.h>

#define TIME_GIGA (1000000000)
#define MAX_TASKS (32)

struct task_par {
    int arg;            /* task argument */
    long wcet;          /* in microseconds */
    int period;         /* in milliseconds */
    int deadline;       /* relative (ms) */
    int priority;       /* in [0,99] */
    int deadline_miss;  /* no. of misses */
    struct timespec at; /* next activ. time */
    struct timespec dl; /* abs. deadline */
};

struct task_par tp[MAX_TASKS];
pthread_t tid[MAX_TASKS];
size_t taskCount = 0;


void time_copy(struct timespec * td,
               struct timespec ts)
{
    td->tv_sec  = ts.tv_sec;
    td->tv_nsec = ts.tv_nsec;
}

void time_mono_diff(struct timespec * t2,
                    const struct timespec * t1)
{
    t2->tv_sec = t1->tv_sec - t2->tv_sec;
    t2->tv_nsec = t1->tv_nsec - t2->tv_nsec;
    if (t2->tv_sec < 0) {
        t2->tv_sec = 0;
        t2->tv_nsec = 0;
    } else if (t2->tv_nsec < 0) {
        if (t2->tv_sec == 0) {
            t2->tv_sec = 0;
            t2->tv_nsec = 0;
        } else {
            t2->tv_sec = t2->tv_sec - 1;
            t2->tv_nsec = t2->tv_nsec + TIME_GIGA;
        }
    }
}

void time_add_ms(struct timespec * t,
                 const int ms)
{
    t->tv_sec += ms / 1000;
    t->tv_nsec += (ms % 1000) * 1000000;
    if (t->tv_nsec > TIME_GIGA) {
        t->tv_nsec -= TIME_GIGA;
        t->tv_sec += 1;
    }
}

int time_cmp(const struct timespec t1,
             const struct timespec t2)
{
    if (t1.tv_sec > t2.tv_sec) return 1;
    if (t1.tv_sec < t2.tv_sec) return -1;
    if (t1.tv_nsec > t2.tv_nsec) return 1;
    if (t1.tv_nsec < t2.tv_nsec) return -1;
    return 0;
}

#ifdef __MACH__
/* emulate clock_nanosleep for CLOCK_MONOTONIC and TIMER_ABSTIME */
int clock_nanosleep_abstime(const struct timespec * req)
{
    struct timespec ts_delta;
    int retval = clock_gettime(CLOCK_MONOTONIC, &ts_delta);
    if (retval == 0) {
        time_mono_diff(&ts_delta, req);
        retval = nanosleep(&ts_delta, NULL);
    }
    return retval;
}
#else /* POSIX */
    /* clock_nanosleep for CLOCK_MONOTONIC and TIMER_ABSTIME */
    #define clock_nanosleep_abstime(req) \
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,(req), NULL)
#endif


int ptask_create(void*(*task)(void *),
                 const int period,
                 const int deadline,
                 const int priority)
{
    pthread_attr_t myatt;
    struct sched_param mypar;

    int id = taskCount++;
    assert(id < MAX_TASKS);

    tp[id].arg = id;
    tp[id].period = period;
    tp[id].deadline = deadline;
    tp[id].priority = priority;
    tp[id].deadline_miss = 0;

    pthread_attr_init(&myatt);
    pthread_attr_setinheritsched(&myatt, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&myatt, SCHED_RR);
    mypar.sched_priority = tp[id].priority;
    pthread_attr_setschedparam(&myatt, &mypar);
    tret = pthread_create(&tid[id], &myatt, task, (void *)(&tp[id]));

    if (tret != 0) return PTASK_ERROR;

    return id;
}

int ptask_id(void *arg)
{
    struct task_par *tp;
    tp = (struct task_par *)arg;
    return tp->arg;
}

void ptask_activate(int i)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    time_copy(&(tp[i].at), t);
    time_copy(&(tp[i].dl), t);
    time_add_ms(&(tp[i].at), tp[i].period);
    time_add_ms(&(tp[i].dl), tp[i].deadline);
}

int ptask_deadline_miss(int i)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (time_cmp(now, tp[i].dl) > 0) {
        tp[i].deadline_miss++;
        return 1;
    }
    return 0;
}

void ptask_wait_for_activation(int i)
{
    clock_nanosleep_abstime(&(tp[i].at));
    time_add_ms(&(tp[i].at), tp[i].period);
    time_add_ms(&(tp[i].dl), tp[i].deadline);
}

void ptask_wait_tasks()
{
    for (int i = 0; i < taskCount; ++i) {
        pthread_join(tid[i], NULL);
    }
}
