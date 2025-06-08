# 🧵 Operating Systems – Exercise 2: User-Level Thread Library

This project implements a **user-level threading library** in C, based on `sigsetjmp`/`siglongjmp`, virtual timers, and manual stack manipulation. It supports preemptive scheduling, sleeping threads, thread blocking/resuming, and quantum tracking.

> Developed as part of the Operating Systems course at Reichman University.

---

## 📁 Project Structure

| File        | Description                                                                 |
|-------------|-----------------------------------------------------------------------------|
| `uthreads.c`| Core implementation of the threading library logic                          |
| `uthreads.h`| Public API and data structure declarations for the threading library        |
| `main.c`    | Sample test program with 4 threads doing different behaviors (commented out)|
| `test1.c`   | Simpler test with 2 threads demonstrating thread creation and termination   |

---

## 🚀 Features

✅ Preemptive context switching using `SIGVTALRM`  
✅ Thread spawn, terminate, block, resume  
✅ Thread sleep for a fixed number of quantums  
✅ Quantum tracking (per-thread and total)  
✅ Manual stack and context management  
✅ Safety using signal masking for atomic thread state transitions

---

## ⚙️ Usage

### 🧪 Compile the test program:
```bash
gcc uthreads.c test1.c -o test_threads -Wall
./test_threads
```

### 💡 Alternate test program:
If you want to use the multi-threaded example in `main.c`:

1. Uncomment the `main()` in `main.c`
2. Compile:
   ```bash
   gcc uthreads.c main.c -o test_main -Wall
   ./test_main
   ```

---

## 🧠 Thread States

| State        | Meaning                                      |
|--------------|----------------------------------------------|
| `THREAD_UNUSED`  | Slot is free                               |
| `THREAD_READY`   | Thread is ready to run                    |
| `THREAD_RUNNING` | Currently executing                       |
| `THREAD_BLOCKED` | Waiting due to sleep/block request        |

---

## 🛠 API Functions

```c
int uthread_init(int quantum_usecs);
int uthread_spawn(thread_entry_point entry_point);
int uthread_terminate(int tid);
int uthread_block(int tid);
int uthread_resume(int tid);
int uthread_sleep(int num_quantums);
int uthread_get_tid(void);
int uthread_get_total_quantums(void);
int uthread_get_quantums(int tid);
```

---

## 📌 Example Output (`test1.c`)
```
Thread 1: 0
Thread 2: 0
Thread 1: 1
Thread 2: 1
...
Done!
```

---

## 👨‍💻 Author

- **Nir Itzik**  
  GitHub: [@niritsiq](https://github.com/niritsiq)

---

## 🔗 Repository

[https://github.com/niritsiq/operating-system---ex2](https://github.com/niritsiq/operating-system---ex2)

---

## 🧼 License

This project is submitted for academic purposes and is **not licensed** for public reuse without permission.
