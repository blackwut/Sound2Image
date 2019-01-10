#ifndef BQUEUE_H
#define BQUEUE_H

#include <stdlib.h>
#include <pthread.h>

#define BQUEUE_SUCCESS 0


#define QUEUE_SIZE 512

typedef struct bqueue {
    void * items[QUEUE_SIZE];
    size_t head;
    size_t tail;
    pthread_mutex_t mux;
    pthread_cond_t cond;
} BQueue;


int bqueue_create(BQueue * q);
void bqueue_lock(BQueue * q);
void bqueue_unlock(BQueue * q);
int bqueue_is_empty_unsafe(BQueue * q);
int bqueue_is_empty(BQueue * q);
int bqueue_enqueue(BQueue * q, void * data);
void * bqueue_dequeue(BQueue * q, const int timeout);
int bqueue_destroy(BQueue * q, void(*item_free)(void *));

#endif
