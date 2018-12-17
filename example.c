#include <stdio.h>
#include <stdlib.h>
#include "ptask.h"

void * task(void * arg) {

    int id;

    id = ptask_id(arg);
    ptask_activate(id);

    while (1) {

        printf("(%d) hello!\n", id);

        if (ptask_deadline_miss(id)) {
            printf("%d) deadline missed!\n", id);
        }

        ptask_wait_for_activation(id);
    }
}

int main(int argc, char * argv[])
{
   for (int i = 0; i < 4; ++i) {
        ptask_create(task, 100, 100, 10);
    }

    ptask_wait_tasks();

    return 0;
}