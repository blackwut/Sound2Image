#ifndef BQUEUE_H
#define BQUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define QUEUE_SIZE 512

typedef struct bqueue {
    void * items[QUEUE_SIZE];

    size_t head;
    size_t tail;

    pthread_mutex_t mux;
} BQueue;

void bqueue_init(BQueue * q);
void bqueue_global_lock(BQueue * q);
void bqueue_enqueue_lock(BQueue * q);
void bqueue_dequeue_lock(BQueue * q);
void bqueue_global_unlock(BQueue * q);
void bqueue_enqueue_unlock(BQueue * q);
void bqueue_dequeue_unlock(BQueue * q);
int bqueue_is_empty(BQueue * q);
void bqueue_enqueue(BQueue * q,
                    void * data);
void * bqueue_dequeue(BQueue * q);
void bqueue_destroy(BQueue * q, void(*item_free)(void *));

#endif
