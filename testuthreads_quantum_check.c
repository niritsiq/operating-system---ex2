
#include "uthreads.h"
#include <stdio.h>
#include <unistd.h>

#define NUM_THREADS 5

void busy_work() {
    for (int i = 0; i < 5; i++) {
        usleep(10000); // simulate work
    }
    uthread_terminate(uthread_get_tid());
}

/*int main() {
    if (uthread_init(100000) == -1) {
        fprintf(stderr, "Failed to initialize uthreads\n");
        return 1;
    }

    int tids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        tids[i] = uthread_spawn(busy_work);
        if (tids[i] == -1) {
            fprintf(stderr, "Failed to spawn thread %d\n", i);
            return 1;
        }
    }

    // Let main thread run for a while to allow others to execute
    for (int i = 0; i < 500; i++) {
        usleep(100000);
    }

    // Calculate sum of all quantums
    int sum = 0;
    for (int i = 0; i < MAX_THREAD_NUM; i++) {
        int q = uthread_get_quantums(i);
        if (q > 0) {
            printf("Thread %d ran %d quantums\n", i, q);
            sum += q;
        }
    }

    int total = uthread_get_total_quantums();
    printf("Total quantums reported: %d\n", total);
    printf("Sum of all threads' quantums: %d\n", sum);

    if (sum == total) {
        printf("✅ SUCCESS: Total equals sum of thread quantums\n");
    } else {
        printf("❌ ERROR: Mismatch between total and sum of thread quantums\n");
    }

    return 0;
}*/
