#ifndef PTASK_H
#define PTASK_H

#define PTASK_ERROR -1

int ptask_create(void*(*task)(void *), int period, int drel, int prio);
int ptask_id(void *arg);
void ptask_activate(int i);
int ptask_deadline_miss(int i);
void ptask_wait_for_activation(int i);
void ptask_wait_tasks();

#endif
