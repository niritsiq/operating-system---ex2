
#include "uthreads.h"
#include <stdio.h>
#include <unistd.h>

void thread_func1() {
    for (int i = 0; i < 3; i++) {
        printf("Thread 1 running, quantum: %d\n", uthread_get_quantums(uthread_get_tid()));
        usleep(1000);
    }
    uthread_terminate(uthread_get_tid());
}

void thread_func2() {
    for (int i = 0; i < 3; i++) {
        printf("Thread 2 running, quantum: %d\n", uthread_get_quantums(uthread_get_tid()));
        usleep(1000);
    }
    uthread_terminate(uthread_get_tid());
}

/*int main() {
    if (uthread_init(100000) == -1) {
        printf("Init failed\n");
        return 1;
    }

    int tid1 = uthread_spawn(thread_func1);
    int tid2 = uthread_spawn(thread_func2);

    if (tid1 == -1 || tid2 == -1) {
        printf("Spawn failed\n");
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        printf("Main thread: tid %d, total quantums: %d\n", uthread_get_tid(), uthread_get_total_quantums());
        usleep(100000);
    }

    printf("Test complete. Main thread terminating.\n");
    return 0;
}*/
