#include "ptask.h"
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <assert.h>
#include "time_utils.h"

//------------------------------------------------------------------------------
// PTASK LOCAL CONSTANTS
//------------------------------------------------------------------------------
#define MAX_TASKS	64	// Maximum number of periodic task that can be created


//------------------------------------------------------------------------------
// PTASK LOCAL STRUCT DEFINITIONS
//------------------------------------------------------------------------------
typedef struct {
	size_t id;					// the internal id of the periodic task
	size_t task_id;				// the id chosen by the user
	size_t period;				// period expressed in milliseconds
	size_t deadline;			// relative deadline expressed in milliseconds
	size_t priority;			// priority in the range 0 (low) to 99 (high)
	struct timespec woet;		// worst observed execution time expressed in ms
	size_t deadline_misses;		// number of deadline misses
	struct timespec at;			// next absolute activation time
	struct timespec dl;			// absolute deadline
} task_par;


//------------------------------------------------------------------------------
// PTASK LOCAL DATA
//------------------------------------------------------------------------------
static task_par tp[MAX_TASKS];		// parameters of created tasks
static pthread_t tid[MAX_TASKS];	// ids of created tasks
static size_t task_count = 0;		// number of created tasks


//------------------------------------------------------------------------------
//
// This function creates a periodic task. The periodic task is implemented with
// a POSIX pthread and scheduled in a Round-Robin fashion.
// All related information of the periodic task are stored into a proper
// structure in order to keep track of it and manage it during the execution.
//
//------------------------------------------------------------------------------
int ptask_create(void * (*task_handler)(void *),
				 const size_t task_id,
				 const size_t period,
				 const size_t deadline,
				 const size_t priority)
{
	int ret;						// return value of pthread_create
	size_t id;						// current internal id of the task
	pthread_attr_t attributes;		// scheduler attributes
	struct sched_param sched;		// scheduler parameters

	assert(task_count < MAX_TASKS);

	id = task_count;
	task_count++;

	tp[id].id = id;
	tp[id].task_id = task_id;
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
	pthread_attr_destroy(&attributes);

	if (ret != 0) {
		return PTASK_ERROR_CREATE;
	}

	return PTASK_SUCCESS;
}


//------------------------------------------------------------------------------
//
// This function extracts the id of the periodic task from the pointer arg that
// contains all related information of that task.
//
//------------------------------------------------------------------------------
size_t ptask_id(const void * arg)
{
	task_par * tp = (task_par *)arg;
	return tp->id;
}


//------------------------------------------------------------------------------
//
// This function extracts the id of the periodic task, provided by the user,
// from the pointer arg that contains all related information of the that task.
//
//------------------------------------------------------------------------------
size_t ptask_task_id(const void * arg)
{
	task_par * tp = (task_par *)arg;
	return tp->task_id;
}


//------------------------------------------------------------------------------
//
// This function activates the periodic task.
// It sets the next absolute activation time to the current time plus the
// relative period.
// It also sets the absolute deadline to the current time plus the relative
// deadline.
//
//------------------------------------------------------------------------------
void ptask_activate(const size_t id)
{
	struct timespec now;		// current time
	time_now(&now);
	time_copy(&(tp[id].at), now);
	time_copy(&(tp[id].dl), now);
	time_add_ms(&(tp[id].at), tp[id].period);
	time_add_ms(&(tp[id].dl), tp[id].deadline);
}


//------------------------------------------------------------------------------
//
// This function checks if the periodic task missed the deadline.
// It takes the current time and compare it to the absolute deadline in order to
// evaluate if the deadline is missed. It also calculate the current woet (worst
// execution observed time).
//
//------------------------------------------------------------------------------
int ptask_deadline_miss(const size_t id)
{
	struct timespec now;		// current time
	struct timespec diff;		// the difference (at - now)

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


//------------------------------------------------------------------------------
//
// This function suspends the periodic task until the next activation.
// When the periodic task is awaken, it updates the absolute activation time and
// the absolute deadline.
//
//------------------------------------------------------------------------------
void ptask_wait_for_activation(const size_t id)
{
	time_nanosleep(tp[id].at);
	time_add_ms(&(tp[id].at), tp[id].period);
	time_add_ms(&(tp[id].dl), tp[id].period);
}


//------------------------------------------------------------------------------
//
// This function returns the woet (worst observed execution time) of the
// periodic task expressed in milliseconds.
//
//------------------------------------------------------------------------------
size_t ptask_get_woet_ms(const size_t id)
{
	return time_to_ms(tp[id].woet);
}


//------------------------------------------------------------------------------
//
// This function provide a simple way to join all the periodic tasks created.
// It checks if the tasks make the join with success. If not it returns 
// PTASK_ERROR_JOIN value. Otherwise it returns PTASK_SUCCESS.
//
//------------------------------------------------------------------------------
int ptask_wait_tasks()
{
	int ret;		// pthread_join return value

	for (int i = 0; i < task_count; ++i) {
		ret = pthread_join(tid[i], NULL);
		if (ret != 0) {
			return PTASK_ERROR_JOIN;
		}
	}
	return PTASK_SUCCESS;
}
