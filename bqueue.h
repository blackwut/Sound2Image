#ifndef BQUEUE_H
#define BQUEUE_H

typedef struct bqueue * BQueue;

BQueue bqueue_create();
void bqueue_lock(BQueue q);
void bqueue_unlock(BQueue q);
int bqueue_is_empty(BQueue q);
void bqueue_enqueue(BQueue q, void * data);
void * bqueue_dequeue(BQueue q);
void bqueue_destroy(BQueue q, void(*data_free)(void *));

void bqueue_test();
void bqueue_test_multithread();

#endif
