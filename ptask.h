#ifndef PTASK_H
#define PTASK_H

#include <stdio.h>

#define PTASK_SUCCESS       0
#define PTASK_DEADLINE_MISS 1

int ptask_create(void * (*task_handler)(void *),
				 const size_t period,
				 const size_t deadline,
				 const size_t priority);
size_t ptask_id(const void *arg);
void ptask_activate(const size_t id);
int ptask_deadline_miss(const size_t id);
void ptask_wait_for_activation(const size_t id);
size_t ptask_get_woet_ms(const size_t id);
void ptask_sleep_ms(const size_t ms);
void ptask_wait_tasks();

#endif
