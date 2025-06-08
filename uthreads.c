#include "uthreads.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>
#include <string.h>
#include <stdbool.h>

#define JB_SP 6
#define JB_PC 7
typedef unsigned long address_t;

static thread_t threads[MAX_THREAD_NUM];
static char thread_stacks[MAX_THREAD_NUM][STACK_SIZE];
static int ready_queue[MAX_THREAD_NUM];
static int ready_front = 0, ready_rear = 0, ready_count = 0;
static int current_thread_id = -1;
static int total_quantums = 0;
static int quantum_usecs = 0;
static bool library_initialized = false;
static struct sigaction original_sigaction;
static struct itimerval timer_config;
static sigset_t sigvtalrm_set;

address_t translate_address(address_t addr) {
    address_t ret;
    asm volatile("xor %%fs:0x30, %0\n"
                 "rol $0x11, %0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

static void mask_sigvtalrm() {
    if (sigprocmask(SIG_BLOCK, &sigvtalrm_set, NULL) == -1) {
        fprintf(stderr, "system error: masking failed\n");
        exit(1);
    }
}

static void unmask_sigvtalrm() {
    if (sigprocmask(SIG_UNBLOCK, &sigvtalrm_set, NULL) == -1) {
        fprintf(stderr, "system error: masking failed\n");
        exit(1);
    }
}

static void enqueue_ready(int tid) {
    if (ready_count >= MAX_THREAD_NUM) return;
    ready_queue[ready_rear] = tid;
    ready_rear = (ready_rear + 1) % MAX_THREAD_NUM;
    ready_count++;
}

static int dequeue_ready() {
    if (ready_count == 0) return -1;
    int tid = ready_queue[ready_front];
    ready_front = (ready_front + 1) % MAX_THREAD_NUM;
    ready_count--;
    return tid;
}

static void remove_from_ready_queue(int tid) {
    int tmp[MAX_THREAD_NUM];
    int count = 0;
    for (int i = 0; i < ready_count; ++i) {
        int index = (ready_front + i) % MAX_THREAD_NUM;
        if (ready_queue[index] != tid) {
            tmp[count++] = ready_queue[index];
        }
    }
    for (int i = 0; i < count; ++i) {
        ready_queue[i] = tmp[i];
    }
    ready_front = 0;
    ready_rear = count;
    ready_count = count;
}

void setup_thread(int tid, char* stack, thread_entry_point entry) {
    address_t sp = (address_t)stack + STACK_SIZE - sizeof(address_t);
    address_t pc = (address_t)entry;
    sigsetjmp(threads[tid].env, 1);
    threads[tid].env->__jmpbuf[JB_SP] = translate_address(sp);
    threads[tid].env->__jmpbuf[JB_PC] = translate_address(pc);
    sigemptyset(&threads[tid].env->__saved_mask);
}

static void wake_sleeping_threads() {
    for (int i = 0; i < MAX_THREAD_NUM; i++) {
        if (threads[i].state == THREAD_BLOCKED &&
            threads[i].sleep_until > 0 &&
            threads[i].sleep_until <= total_quantums) {
            threads[i].sleep_until = 0;
            threads[i].state = THREAD_READY;
            enqueue_ready(i);
        }
    }
}

void schedule_next();

void timer_handler(int signum) {
    (void)signum;
    
    wake_sleeping_threads();
    
    if (threads[current_thread_id].state == THREAD_RUNNING) {
        threads[current_thread_id].state = THREAD_READY;
        enqueue_ready(current_thread_id);
    }
    
    schedule_next();
}

void schedule_next() {
    mask_sigvtalrm();
    int next_tid = dequeue_ready();
    if (next_tid == -1) {
        unmask_sigvtalrm();
        return;
    }

    int prev_tid = current_thread_id;
    current_thread_id = next_tid;
    threads[next_tid].state = THREAD_RUNNING;
    
    // Increment quantum counters when thread starts running
    threads[next_tid].quantums++;
    total_quantums++;

    // Configure timer for one-shot quantum duration
    timer_config.it_value.tv_sec = quantum_usecs / 1000000;
    timer_config.it_value.tv_usec = quantum_usecs % 1000000;
    timer_config.it_interval.tv_sec = 0;     // One-shot timer
    timer_config.it_interval.tv_usec = 0;   // No automatic restart
    setitimer(ITIMER_VIRTUAL, &timer_config, NULL);

    if (prev_tid != -1 && sigsetjmp(threads[prev_tid].env, 1) == 0) {
        siglongjmp(threads[next_tid].env, 1);
    }

    unmask_sigvtalrm();
}

int uthread_init(int quantum_usecs_param) {
    if (library_initialized || quantum_usecs_param <= 0) {
        fprintf(stderr, "thread library error: bad init\n");
        return -1;
    }

    quantum_usecs = quantum_usecs_param;
    sigemptyset(&sigvtalrm_set);
    sigaddset(&sigvtalrm_set, SIGVTALRM);
    mask_sigvtalrm();

    for (int i = 0; i < MAX_THREAD_NUM; i++) {
        threads[i].tid = i;
        threads[i].state = THREAD_UNUSED;
        threads[i].quantums = 0;
        threads[i].entry = NULL;
        threads[i].sleep_until = 0;
    }

    struct sigaction sa = {0};
    sa.sa_handler = timer_handler;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGVTALRM);
    sa.sa_flags = 0;
    if (sigaction(SIGVTALRM, &sa, &original_sigaction) == -1) {
        perror("sigaction");
        exit(1);
    }

    threads[0].state = THREAD_RUNNING;
    threads[0].quantums = 1;  // Main thread starts with quantum 1
    current_thread_id = 0;
    total_quantums = 1;       // Total starts with 1
    sigsetjmp(threads[0].env, 1);
    sigdelset(&threads[0].env[0].__saved_mask, SIGVTALRM);

    // Configure initial timer
    timer_config.it_value.tv_sec = quantum_usecs / 1000000;
    timer_config.it_value.tv_usec = quantum_usecs % 1000000;
    timer_config.it_interval.tv_sec = 0;     // One-shot timer
    timer_config.it_interval.tv_usec = 0;   // No automatic restart
    setitimer(ITIMER_VIRTUAL, &timer_config, NULL);

    library_initialized = true;
    unmask_sigvtalrm();
    return 0;
}

int uthread_spawn(thread_entry_point entry) {
    if (!library_initialized || entry == NULL) {
        fprintf(stderr, "thread library error: bad spawn\n");
        return -1;
    }

    mask_sigvtalrm();
    int tid = -1;
    for (int i = 1; i < MAX_THREAD_NUM; i++) {
        if (threads[i].state == THREAD_UNUSED) {
            tid = i;
            break;
        }
    }
    if (tid == -1) {
        fprintf(stderr, "thread library error: maximum threads reached\n");
        unmask_sigvtalrm();
        return -1;
    }

    threads[tid].state = THREAD_READY;
    threads[tid].entry = entry;
    threads[tid].quantums = 0;  // New thread starts with 0 quantums
    threads[tid].sleep_until = 0;
    setup_thread(tid, thread_stacks[tid], entry);
    enqueue_ready(tid);

    unmask_sigvtalrm();
    return tid;
}

int uthread_terminate(int tid) {
    if (!library_initialized || tid < 0 || tid >= MAX_THREAD_NUM || threads[tid].state == THREAD_UNUSED) {
        fprintf(stderr, "thread library error: invalid terminate\n");
        return -1;
    }

    mask_sigvtalrm();

    if (tid == 0) {
        // Terminating main thread terminates entire process
        struct itimerval stop_timer = {{0, 0}, {0, 0}};
        setitimer(ITIMER_VIRTUAL, &stop_timer, NULL);
        exit(0);
    }

    if (threads[tid].state == THREAD_READY) {
        remove_from_ready_queue(tid);
    }

    threads[tid].state = THREAD_UNUSED;

    if (tid == current_thread_id) {
        schedule_next();
    } else {
        unmask_sigvtalrm();
    }

    return 0;
}

int uthread_block(int tid) {
    if (!library_initialized || tid <= 0 || tid >= MAX_THREAD_NUM || threads[tid].state == THREAD_UNUSED) {
        // tid <= 0 prevents blocking main thread (tid=0) per requirements
        fprintf(stderr, "thread library error: invalid block\n");
        return -1;
    }

    mask_sigvtalrm();

    if (threads[tid].state == THREAD_BLOCKED) {
        unmask_sigvtalrm();
        return 0;
    }

    if (threads[tid].state == THREAD_READY) {
        remove_from_ready_queue(tid);
    }

    threads[tid].state = THREAD_BLOCKED;

    if (tid == current_thread_id) {
        schedule_next();
    }

    unmask_sigvtalrm();
    return 0;
}

int uthread_resume(int tid) {
    if (!library_initialized || tid <= 0 || tid >= MAX_THREAD_NUM || threads[tid].state == THREAD_UNUSED) {
        return -1;
    }

    mask_sigvtalrm();

    if (threads[tid].state == THREAD_BLOCKED &&
        (threads[tid].sleep_until == 0 || threads[tid].sleep_until <= total_quantums)) {
        threads[tid].sleep_until = 0;
        threads[tid].state = THREAD_READY;
        enqueue_ready(tid);
    }

    unmask_sigvtalrm();
    return 0;
}

int uthread_sleep(int num_quantums) {
    if (!library_initialized || current_thread_id == 0 || num_quantums <= 0) {
        // Main thread (tid=0) cannot sleep per requirements
        return -1;
    }

    mask_sigvtalrm();
    threads[current_thread_id].state = THREAD_BLOCKED;
    threads[current_thread_id].sleep_until = total_quantums + num_quantums + 1;
    schedule_next();
    return 0;
}

int uthread_get_tid() {
    return current_thread_id;
}

int uthread_get_total_quantums() {
    return total_quantums;
}

int uthread_get_quantums(int tid) {
    if (!library_initialized || tid < 0 || tid >= MAX_THREAD_NUM || threads[tid].state == THREAD_UNUSED) {
        return -1;
    }
    return threads[tid].quantums;
}