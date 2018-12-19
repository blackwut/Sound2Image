#include "bqueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct bnode {
    struct bnode * next;
    void * data;
};

struct bqueue {
    struct bnode * head;
    struct bnode * tail;
    size_t size;

    pthread_mutex_t * mux;
};

typedef struct bnode * BNode;


BNode create_node(void * data) {
    BNode node = (BNode) calloc(1, sizeof(struct bnode));
    node->next = NULL;
    node->data = data;
    return node;
}

BQueue bqueue_create()
{
    BQueue q = (BQueue) calloc(1, sizeof(struct bqueue));
    q->tail = q->head = NULL;
    q->size = 0;
    q->mux = (pthread_mutex_t *) calloc(1, sizeof(pthread_mutex_t));
    pthread_mutex_init(q->mux, NULL);
    return q;
}

void bqueue_lock(BQueue q)
{
    pthread_mutex_lock(q->mux);
}

void bqueue_unlock(BQueue q)
{
    pthread_mutex_unlock(q->mux);
}

int bqueue_is_empty(BQueue q)
{
    bqueue_lock(q);
    int size = q->size == 0;
    bqueue_unlock(q);
    return size;
}

void bqueue_enqueue(BQueue q,
                   void * data)
{
    BNode node =  create_node(data);

    bqueue_lock(q);
    if (q->head == NULL && q->tail == NULL) {
        q->head = node;
    } else {
        q->tail->next = node;
    }

    q->tail = node;
    q->size++;
    bqueue_unlock(q);
}

void * bqueue_dequeue(BQueue q)
{
    void * data = NULL;
    
    bqueue_lock(q);
   
    BNode node = q->head;
    if (node != NULL) {
        q->head = q->head->next;
        if (q->head == NULL) {
            q->tail = NULL;
        }
        q->size--;
        data = node->data;
        free(node);
    }

    bqueue_unlock(q);

    return data;
}

void bqueue_destroy(BQueue q, void(*data_free)(void *))
{
    bqueue_lock(q);
    while (q->head != NULL) {
        BNode node = q->head;
        if (data_free != NULL) data_free(node->data);
        free(node);
        q->head = q->head->next;
    }
    bqueue_unlock(q);
    pthread_mutex_destroy(q->mux);
    free(q->mux);
    free(q);
}


#include <time.h>
#define TEST_ENQUEUE 0
#define TEST_DEQUEUE 1
#define TEST_SIZE 1000
#define TEST_TIDS 8

typedef struct test_args {
    BQueue q;
    int type;
    int tid;
} * Test_args;

void bqueue_test()
{
    int * data_null = (int *)-1;
    int * data_one = (int *)1;
    int * data_two = (int *)2;
    int * data_three = (int *)3;

    BQueue q = bqueue_create();

    bqueue_enqueue(q, data_one);
    bqueue_enqueue(q, data_two);
    bqueue_enqueue(q, data_three);

    data_one = bqueue_dequeue(q);
    data_two = bqueue_dequeue(q);
    data_three = bqueue_dequeue(q);
    data_null = bqueue_dequeue(q);


    printf("TEST: enqueue 3 items, dequeue 3 items\n");
    printf("data_one: %d\n", (int)data_one);
    printf("data_two: %d\n", (int)data_two);
    printf("data_three: %d\n", (int)data_three);
    printf("data_null: %s\n", data_null == NULL ? "NULL" : "not NULL");


    printf("TEST: enqueue 1 item, dequeue 1 item, enqueue 2 items, dequeue 2 items\n");
    bqueue_enqueue(q, data_one);
    data_one = bqueue_dequeue(q);
    bqueue_enqueue(q, data_two);
    bqueue_enqueue(q, data_three);
    data_two = bqueue_dequeue(q);
    data_three = bqueue_dequeue(q);
    data_null = bqueue_dequeue(q);

    printf("data_one: %d\n", (int)data_one);
    printf("data_two: %d\n", (int)data_two);
    printf("data_three: %d\n", (int)data_three);
    printf("data_null: %s\n", data_null == NULL ? "NULL" : "not NULL");

    printf("I'm destroing the queue!\n");
    bqueue_destroy(q, free);
    q = NULL;
}


void * test_thread(void * arg)
{
    Test_args args = (Test_args)arg;
    printf("Thread %d START\n", args->tid);

    for (int i = 0; i < TEST_SIZE; ++i) {
        if (args->type == TEST_ENQUEUE) {
            float * data = (float *) malloc(sizeof(float));
            bqueue_enqueue(args->q, data);
        } else {
            float * data = bqueue_dequeue(args->q);
            free(data);
        }
    }

    printf("Thread %d END\n", args->tid);

    return NULL;
}

void bqueue_test_multithread()
{
    srand(time(NULL));

    pthread_t tids[TEST_TIDS];
    Test_args args[TEST_TIDS];
    BQueue q = bqueue_create();

    for (int i = 0; i < TEST_TIDS; ++i) {
        args[i] = (Test_args)calloc(1, sizeof(struct test_args));
        args[i]->q = q;
        args[i]->type = i & 1;
        args[i]->tid = i;

        pthread_create(&tids[i], NULL, test_thread, args[i]);
    }

    for (int i = 0; i < TEST_TIDS; ++i) {
        pthread_join(tids[i], NULL);
        free(args[i]);
    }

    bqueue_destroy(q, free);
    q = NULL;
}
