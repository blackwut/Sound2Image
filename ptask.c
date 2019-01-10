#include "ptask.h"
#include <pthread.h>
#include <sched.h>
#include <assert.h>
#include "timeutil.h"

#define MAX_TASKS (32)


struct task_par {
    int id;             // task argument
    long wcet;          // in microseconds
    int period;         // in milliseconds
    int deadline;       // relative (ms)
    int priority;       // in [0,99]
    int deadline_miss;  // no. of misses
    struct timespec at; // next activ. time
    struct timespec dl; // abs. deadline
};

struct task_par tp[MAX_TASKS];
pthread_t tid[MAX_TASKS];
size_t taskCount = 0;


int ptask_create(void*(*task)(void *),
                 const int period,
                 const int deadline,
                 const int priority)
{
    pthread_attr_t myatt;
    struct sched_param mypar;
    int tret;

    int id = taskCount++;
    assert(id < MAX_TASKS);

    tp[id].id = id;
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
    return tp->id;
}

void ptask_activate(int id)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    time_copy(&(tp[id].at), t);
    time_copy(&(tp[id].dl), t);
    time_add_ms(&(tp[id].at), tp[id].period);
    time_add_ms(&(tp[id].dl), tp[id].deadline);
}

int ptask_deadline_miss(int id)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (time_cmp(now, tp[id].dl) > 0) {
        tp[id].deadline_miss++;
        return 1;
    }
    return 0;
}

void ptask_wait_for_activation(int id)
{
    clock_nanosleep_abstime(&(tp[id].at));
    time_add_ms(&(tp[id].at), tp[id].period);
    time_add_ms(&(tp[id].dl), tp[id].deadline);
    printf("(%d) active!\n", id);
}

void ptask_wait_tasks()
{
    for (int i = 0; i < taskCount; ++i) {
        pthread_join(tid[i], NULL);
    }
}
