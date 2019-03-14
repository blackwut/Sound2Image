//------------------------------------------------------------------------------
//
// PTASK
// LIBRARY TO SIMPLIFY THE MANAGEMENT OF PERIODIC TASKS
//
//------------------------------------------------------------------------------

#ifndef PTASK_H
#define PTASK_H

#include <stdio.h>


//------------------------------------------------------------------------------
// CONSTANTS
//------------------------------------------------------------------------------
#define PTASK_SUCCESS		0
#define PTASK_DEADLINE_MISS	1


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function creates a new periodic task specifing the task_handler, period,
// deadline and priority. The task is scheduled with a Round-Robin scheduler.
//
// PARAMETERS
// task_handler: routine of the task
// period: period of the task
// deadline: deadline of the task. It must be less than or equal to period
// priority: priority of the task. The value must be in the range [0, 99]
//
// RETURN
// If the periodic task is created, it returns the id of the periodic task.
//
// ERROR
// If the periodic task can not be created, the program exits with
// EXIT_PTHREAD_CREATE value.
//
//------------------------------------------------------------------------------
int ptask_create(void * (*task_handler)(void *),
				 const size_t period,
				 const size_t deadline,
				 const size_t priority);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns the id of the periodic task.
//
// PARAMETERS
// arg: the arguments provided to the task_handler routine
//
// RETURN
// The id of the periodic task
//
//------------------------------------------------------------------------------
size_t ptask_id(const void *arg);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function activates the periodic task.
// This function must be called once and before the periodic business code of
// the task.
//
// PARAMETES
// id: the periodic task id
//
//------------------------------------------------------------------------------
void ptask_activate(const size_t id);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function checks if the periodic task missed the deadline and update its
// woet (worst observed execution time).
//
// PARAMETES
// id: the periodic task id
//
// RETURN
// In case of deadline miss it returns PTASK_DEADLINE_MISS.
// Otherwhise it returns PTASK_SUCCESS.
//
//------------------------------------------------------------------------------
int ptask_deadline_miss(const size_t id);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function suspends the periodic task until the next activation.
//
// PARAMETERS
// id: the periodic task id
//
//------------------------------------------------------------------------------
void ptask_wait_for_activation(const size_t id);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns the woet (worst observed execution time) of the
// periodic task expressed in milliseconds.
//
// PARAMETERS
// id: the periodic task id
//
// RETURN
// The woet expressed in milliseconds
//
//------------------------------------------------------------------------------
size_t ptask_get_woet_ms(const size_t id);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function is a help function that provide a simple way to waits the
// completation of all periodic tasks created in the program lifetime.
//
// ERROR
// If the periodic task can not be joined for some reasons, the program exits
// with EXIT_PTHREAD_JOIN value.
//
//------------------------------------------------------------------------------
void ptask_wait_tasks();


#endif
