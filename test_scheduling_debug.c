
#include "uthreads.h"
#include <stdio.h>
#include <unistd.h>

void thread_func() {
    for (int i = 0; i < 5; i++) {
        printf("Thread %d running (quantum %d)\n", uthread_get_tid(), uthread_get_quantums(uthread_get_tid()));
        usleep(50000); // simulate some work
    }
    uthread_terminate(uthread_get_tid());
}

/*int main() {
    if (uthread_init(100000) == -1) {
        printf("uthread_init failed\n");
        return 1;
    }

    for (int i = 0; i < 3; i++) {
        int tid = uthread_spawn(thread_func);
        if (tid == -1) {
            printf("uthread_spawn failed\n");
            return 1;
        }
    }

    // Let main thread live long enough to see others run
    for (int i = 0; i < 30; i++) {
        printf("Main thread quantum: %d\n", uthread_get_quantums(0));
        usleep(100000);
    }

    return 0;
}*/
