#include "bqueue.h"

#include <assert.h>

void bqueue_init(BQueue * q)
{
    assert(q != NULL);

    for (size_t i = 0; i < QUEUE_SIZE; ++i) {
        q->items[i] = NULL;
    }
    q->head = 0;
    q->tail = 0;
    pthread_mutex_init(&(q->mux), NULL);
}

void bqueue_lock(BQueue * q)
{
    assert(q != NULL);
    pthread_mutex_lock(&(q->mux));
}

void bqueue_unlock(BQueue * q)
{
    assert(q != NULL);
    pthread_mutex_unlock(&(q->mux));
}


int bqueue_is_empty(BQueue * q)
{
    assert(q != NULL);
    int isEmpty;

    bqueue_lock(q);
    isEmpty = (q->head == q->tail);
    bqueue_unlock(q);

    return isEmpty;
}

void bqueue_enqueue(BQueue * q,
                    void * data)
{
    assert(q != NULL);

    bqueue_lock(q);
    assert(q->head <= QUEUE_SIZE);
    q->items[q->head] = data;
    q->head = q->head + 1;
    bqueue_unlock(q);
}

void * bqueue_dequeue(BQueue * q)
{
    assert(q != NULL);

    void * data = NULL;

    bqueue_lock(q);
    if (q->tail < q->head) {
        data = q->items[q->tail];
        q->tail = q->tail + 1;
    }
    bqueue_unlock(q);

    return data;
}

void bqueue_destroy(BQueue * q, void(*item_free)(void *))
{
    assert(q != NULL);

    bqueue_lock(q);
    for (size_t i = q->tail; i < q->head; ++i) {
        item_free(q->items[i]);
    }
    bqueue_unlock(q);

    pthread_mutex_destroy(&q->mux);
}
