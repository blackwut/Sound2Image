#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "bqueue.h"


#define TEST_ENQUEUE 0
#define TEST_DEQUEUE 1
#define TEST_SIZE 100
#define TEST_TIDS 4     // must be a multiple of 2

typedef struct test_args {
    BQueue * q;
    int type;
    int tid;
    int items[TEST_SIZE];
} Test_args;


void zeroing(int * items, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        items[i] = 0;
    }
}


void bqueue_test()
{
    int * data_null = (int *)-1;
    int * data_one = (int *)1;
    int * data_two = (int *)2;
    int * data_three = (int *)3;

    BQueue q;
    bqueue_create(&q);

    bqueue_enqueue(&q, data_one);
    bqueue_enqueue(&q, data_two);
    bqueue_enqueue(&q, data_three);

    data_one = bqueue_dequeue(&q, 0);
    data_two = bqueue_dequeue(&q, 0);
    data_three = bqueue_dequeue(&q, 0);
    data_null = bqueue_dequeue(&q, 1);


    printf("TEST: enqueue 3 items, dequeue 3 items\n");
    printf("data_one: %d\n", (int)data_one);
    printf("data_two: %d\n", (int)data_two);
    printf("data_three: %d\n", (int)data_three);
    printf("data_null: %s\n", data_null == NULL ? "NULL" : "not NULL");


    printf("TEST: enqueue 1 item, dequeue 1 item, enqueue 2 items, dequeue 2 items\n");
    bqueue_enqueue(&q, data_one);
    data_one = bqueue_dequeue(&q, 0);
    bqueue_enqueue(&q, data_two);
    bqueue_enqueue(&q, data_three);
    data_two = bqueue_dequeue(&q, 0);
    data_three = bqueue_dequeue(&q, 0);
    data_null = bqueue_dequeue(&q, 1);

    printf("data_one: %d\n", (int)data_one);
    printf("data_two: %d\n", (int)data_two);
    printf("data_three: %d\n", (int)data_three);
    printf("data_null: %s\n", data_null == NULL ? "NULL" : "not NULL");

    printf("I'm destroing the queue!\n");
    bqueue_destroy(&q, free);
}


void * test_thread(void * arg)
{
    Test_args * args = (Test_args *)arg;

    if (args->type == TEST_DEQUEUE) {
        sleep(1);
    }

    printf("Thread %d START\n", args->tid);

    for (size_t i = 0; i < TEST_SIZE; ++i) {
        if (args->type == TEST_ENQUEUE) {
            int * data = (int *) calloc(1, sizeof(int));
            *data = (int)i;
            bqueue_enqueue(args->q, data);
        } else {

            int * data = (int *)bqueue_dequeue(args->q, 0);
            if (data != NULL) {
                args->items[(int)*data]++;
                free(data);
            }
        }
    }

    printf("Thread %d END\n", args->tid);

    return NULL;
}

void bqueue_test_multithread()
{
    int test_items[TEST_SIZE];
    zeroing(test_items, TEST_SIZE);
    pthread_t tids[TEST_TIDS];
    Test_args args[TEST_TIDS];
    BQueue q;
    bqueue_create(&q);

    for (size_t i = 0; i < TEST_TIDS; ++i) {
        args[i].q = &q;
        args[i].type = i & 1;
        args[i].tid = i;
        zeroing(test_items, TEST_SIZE);

        pthread_create(&tids[i], NULL, test_thread, &(args[i]));
    }

    for (size_t i = 0; i < TEST_TIDS; ++i) {
        pthread_join(tids[i], NULL);
    }

    for (size_t i = 0; i < TEST_TIDS; ++i) {
        for (size_t j = 0; j < TEST_SIZE; ++j) {
            test_items[j] += args[i].items[j];
        }
    }

    for (size_t i = 0; i < TEST_SIZE; ++i) {
        int item = test_items[i];
        if (item != TEST_TIDS >> 1) {
            printf("items[%zu] = %d instead of %d\n", i, item, TEST_TIDS >> 1);
        }
    }

    bqueue_destroy(&q, free);
}


int main(int argc, char * argv[])
{
    bqueue_test();
    bqueue_test_multithread();

    return 0;
}
