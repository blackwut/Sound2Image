//------------------------------------------------------------------------------
//
// PTASK
//
// LIBRARY TO SIMPLIFY THE MANAGEMENT OF PERIODIC TASKS
//
//------------------------------------------------------------------------------
#ifndef PTASK_H
#define PTASK_H


#include <stdio.h>


//------------------------------------------------------------------------------
// PTASK GLOBAL CONSTANTS
//------------------------------------------------------------------------------
#define PTASK_SUCCESS		0
#define PTASK_DEADLINE_MISS	1
#define PTASK_ERROR_CREATE	2
#define PTASK_ERROR_JOIN	3


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function creates a new periodic task specifing the task_handler, period,
// deadline and priority. The task is scheduled with a Round-Robin scheduler.
//
// PARAMETERS
// task_handler: routine of the task
// user_id: the id associated by the user to the created task
// period: period of the task
// deadline: deadline of the task. It must be less than or equal to period
// priority: priority of the task. The value must be in the range [0, 99]
//
// RETURN
// If an error occurred, it returns PTASK_ERROR_CREATE.
// Otherwise it returns PTASK_SUCCESS.
//
//
//------------------------------------------------------------------------------
int ptask_create(void * (*task_handler)(void *),
				 const size_t user_id,
				 const size_t period,
				 const size_t deadline,
				 const size_t priority);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns the internal id of the periodic task.
//
// PARAMETERS
// arg: the arguments provided to the task_handler routine
//
// RETURN
// The internal id of the periodic task
//
//------------------------------------------------------------------------------
size_t ptask_id(const void *arg);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns the id of the periodic task provided by the user.
//
// PARAMETERS
// arg: the arguments provided to the task_handler routine
//
// RETURN
// The id of the periodic task provided by the user
//
//------------------------------------------------------------------------------
size_t ptask_user_id(const void *arg);

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
// RETURN
// If an error occurred, it returns PTASK_ERROR_JOIN.
// Otherwise it returns PTASK_SUCCESS.
//
//------------------------------------------------------------------------------
int ptask_wait_tasks();


#endif
