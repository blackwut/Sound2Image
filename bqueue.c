#include "bqueue.h"
#include <assert.h>
#include <errno.h>
#include "timeutil.h"


int bqueue_create(BQueue * q)
{
    assert(q != NULL);

    for (size_t i = 0; i < QUEUE_SIZE; ++i) {
        q->items[i] = NULL;
    }
    q->head = 0;
    q->tail = 0;
    pthread_mutex_init(&(q->mux), NULL);
    pthread_cond_init(&q->cond, NULL);

    return BQUEUE_SUCCESS;
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

int bqueue_is_empty_unsafe(BQueue * q)
{
    return (q->head == q->tail);
}

int bqueue_is_empty(BQueue * q)
{
    assert(q != NULL);
    int isEmpty;

    bqueue_lock(q);
    isEmpty = bqueue_is_empty_unsafe(q);
    bqueue_unlock(q);

    return isEmpty;
}

int bqueue_enqueue(BQueue * q,
                    void * data)
{
    assert(q != NULL);

    bqueue_lock(q);
    assert(q->head <= QUEUE_SIZE);
    q->items[q->head] = data;
    if (bqueue_is_empty_unsafe(q))
        pthread_cond_broadcast(&q->cond);
    q->head = q->head + 1;
    bqueue_unlock(q);

    return BQUEUE_SUCCESS;
}

void * bqueue_dequeue(BQueue * q, const int timeout)
{
    assert(q != NULL);
    int ret = 0;
    struct timespec abstimeout;
    void * data = NULL;

    if (timeout > 0) {
        int retval = clock_gettime(CLOCK_MONOTONIC, &abstimeout);
        if (retval == 0) {
            time_add_ms(&abstimeout, timeout);
        }
    }

    bqueue_lock(q);
    while (bqueue_is_empty_unsafe(q) && (ret != ETIMEDOUT)) {
        if (timeout > 0) {
            ret = pthread_cond_timedwait(&q->cond, &q->mux, &abstimeout);
            if (ret == EINVAL) {
                printf("ERROR in timeout format!\n");
            }
        } else {
            pthread_cond_wait(&q->cond, &q->mux);
        }
    }

    if (ret == ETIMEDOUT) {
        bqueue_unlock(q);
        printf("TIMEOUT!\n");
        return NULL;
    }

    if (q->tail < q->head) {
        data = q->items[q->tail];
        q->tail = q->tail + 1;
    }
    bqueue_unlock(q);

    return data;
}

int bqueue_destroy(BQueue * q, void(*item_free)(void *))
{
    assert(q != NULL);

    bqueue_lock(q);
    for (size_t i = q->tail; i < q->head; ++i) {
        item_free(q->items[i]);
    }
    bqueue_unlock(q);

    pthread_mutex_destroy(&q->mux);
    pthread_cond_destroy(&q->cond);

    return BQUEUE_SUCCESS;
}
