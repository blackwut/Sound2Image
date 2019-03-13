#include "ptask.h"
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <assert.h>
#include "common.h"
#include "time_utils.h"

#define _GNU_SOURCE

#define MAX_TASKS 64

struct task_par {
	size_t id;					// task id
	size_t period;				// period in ms
	size_t deadline;			// relative deadline in ms
	size_t priority;			// between 0 (low) and 99 (high)
	struct timespec woet;		// worst observed execution time
	size_t deadline_misses;		// number of misses
	struct timespec at;			// next activivation time
	struct timespec dl;			// absolute deadline
};

static struct task_par tp[MAX_TASKS];
static pthread_t tid[MAX_TASKS];
static size_t task_count = 0;


int ptask_create(void * (*task_handler)(void *),
				 const size_t period,
				 const size_t deadline,
				 const size_t priority)
{
	int ret;
	size_t id;
	pthread_attr_t attributes;
	struct sched_param sched;

	id = task_count;
	task_count++;
	assert(id < MAX_TASKS);

	tp[id].id = id;
	tp[id].period = period;
	tp[id].deadline = deadline;
	tp[id].priority = priority;
	tp[id].deadline_misses = 0;

	pthread_attr_init(&attributes);
	pthread_attr_setinheritsched(&attributes, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attributes, SCHED_RR);
	sched.sched_priority = tp[id].priority;
	pthread_attr_setschedparam(&attributes, &sched);
	ret = pthread_create(&tid[id], &attributes, task_handler, (void *)(&tp[id]));

	if (ret != 0) {
		DLOG("Error: pthread_create() with code: %d\n", ret);
		exit(EXIT_PTHREAD_CREATE);
	}

	return id;
}

size_t ptask_id(const void * arg)
{
	struct task_par * tp = (struct task_par *)arg;
	return tp->id;
}

void ptask_activate(const size_t id)
{
	struct timespec now;
	time_now(&now);
	time_copy(&(tp[id].at), now);
	time_copy(&(tp[id].dl), now);
	time_add_ms(&(tp[id].at), tp[id].period);
	time_add_ms(&(tp[id].dl), tp[id].deadline);
}

int ptask_deadline_miss(const size_t id)
{
	struct timespec now;
	struct timespec diff;

	time_now(&now);
	time_diff(&diff, tp[id].at, now);

	if (time_cmp(diff, tp[id].woet) > 0) {
		tp[id].woet = diff;
	}

	if (time_cmp(now, tp[id].dl) > 0) {
		tp[id].deadline_misses++;
		return PTASK_DEADLINE_MISS;
	}
	return PTASK_SUCCESS;
}

void ptask_wait_for_activation(const size_t id)
{
	time_nanosleep(tp[id].at);
	time_add_ms(&(tp[id].at), tp[id].period);
	time_add_ms(&(tp[id].dl), tp[id].period);
}

size_t ptask_get_woet_ms(const size_t id)
{
	return time_to_ms(tp[id].woet);
}

void ptask_sleep_ms(const size_t ms)
{
	struct timespec now;
	time_now(&now);
	time_add_ms(&now, ms);
	time_nanosleep(now);
}

void ptask_wait_tasks()
{
	for (int i = 0; i < task_count; ++i) {
		pthread_join(tid[i], NULL);
	}
}
