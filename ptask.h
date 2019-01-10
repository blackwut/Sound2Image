#ifndef PTASK_H
#define PTASK_H

#define PTASK_ERROR -1

int ptask_create(void*(*task)(void *), int period, int drel, int prio);
int ptask_id(void *arg);
void ptask_activate(int id);
int ptask_deadline_miss(int id);
void ptask_wait_for_activation(int id);
void ptask_wait_tasks();

#endif
