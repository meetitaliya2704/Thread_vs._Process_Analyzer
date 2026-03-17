#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

// ─────────────────────────────────────────
// CONFIGURATION
// ─────────────────────────────────────────
#define ARRAY_SIZE  1000000
#define NUM_THREADS 4
#define NUM_PROCS   4
#define CSV_FILE    "results.csv"

// ─────────────────────────────────────────
// THREAD DATA
// ─────────────────────────────────────────
typedef struct {
    long long *array;
    int start;
    int end;
    long long result;
} ThreadData;

// ─────────────────────────────────────────
// THREAD WORKER — sums its chunk
// ─────────────────────────────────────────
void *thread_sum(void *arg) {
    ThreadData *d = (ThreadData *)arg;
    d->result = 0;
    for (int i = d->start; i < d->end; i++)
        d->result += d->array[i];
    pthread_exit(NULL);
}

// ─────────────────────────────────────────
// RUN THREADS — splits array across 4 threads
// ─────────────────────────────────────────
long long run_threads(long long *array) {
    pthread_t   threads[NUM_THREADS];
    ThreadData  data[NUM_THREADS];
    int chunk = ARRAY_SIZE / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        data[i].array  = array;
        data[i].start  = i * chunk;
        data[i].end    = (i == NUM_THREADS-1) ? ARRAY_SIZE : (i+1)*chunk;
        data[i].result = 0;
        pthread_create(&threads[i], NULL, thread_sum, &data[i]);
    }

    long long total = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total += data[i].result;
    }
    return total;
}

// ─────────────────────────────────────────
// RUN PROCESSES — forks 4 child processes
// ─────────────────────────────────────────
long long run_processes(long long *array) {
    int pipes[NUM_PROCS][2];
    int chunk = ARRAY_SIZE / NUM_PROCS;

    // Create pipes
    for (int i = 0; i < NUM_PROCS; i++)
        pipe(pipes[i]);

    // Fork children
    for (int i = 0; i < NUM_PROCS; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            // Child — sum its chunk
            int start     = i * chunk;
            int end       = (i == NUM_PROCS-1) ? ARRAY_SIZE : (i+1)*chunk;
            long long sum = 0;

            for (int j = start; j < end; j++)
                sum += array[j];

            // Send result to parent
            close(pipes[i][0]);
            write(pipes[i][1], &sum, sizeof(long long));
            close(pipes[i][1]);
            exit(0);
        }
    }

    // Parent — collect results
    long long total = 0;
    for (int i = 0; i < NUM_PROCS; i++) {
        long long result = 0;
        close(pipes[i][1]);
        read(pipes[i][0], &result, sizeof(long long));
        close(pipes[i][0]);
        total += result;
    }

    for (int i = 0; i < NUM_PROCS; i++)
        wait(NULL);

    return total;
}

// ─────────────────────────────────────────
// GET TIME in milliseconds
// ─────────────────────────────────────────
double get_time_ms() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1000000.0;
}

// ─────────────────────────────────────────
// GET CPU TICKS from /proc/self/stat
// ─────────────────────────────────────────
long get_cpu_ticks() {
    FILE *f = fopen("/proc/self/stat", "r");
    if (!f) return 0;

    int pid; char comm[256]; char state;
    int ppid, pgrp, session, tty, tpgid;
    unsigned flags;
    long minflt, cminflt, majflt, cmajflt;
    long utime, stime;

    fscanf(f, "%d %s %c %d %d %d %d %d %u "
              "%ld %ld %ld %ld %ld %ld",
           &pid, comm, &state,
           &ppid, &pgrp, &session,
           &tty, &tpgid, &flags,
           &minflt, &cminflt,
           &majflt, &cmajflt,
           &utime, &stime);
    fclose(f);
    return utime + stime;
}

// ─────────────────────────────────────────
// CALCULATE CPU USAGE %
// ─────────────────────────────────────────
double get_cpu_usage(long ticks_before, long ticks_after, double elapsed_ms) {
    long   ticks_used   = ticks_after - ticks_before;
    long   ticks_per_sec = sysconf(_SC_CLK_TCK);
    double cpu_ms       = (ticks_used * 1000.0) / ticks_per_sec;
    if (elapsed_ms <= 0) return 0.0;
    return (cpu_ms / elapsed_ms) * 100.0;
}

// ─────────────────────────────────────────
// SAVE TO CSV
// ─────────────────────────────────────────
void save_csv(char *mode, double time_ms, double cpu) {
    FILE *f = fopen(CSV_FILE, "a");
    if (!f) return;
    fprintf(f, "%s,%d,%.4f,%.2f\n",
            mode, ARRAY_SIZE, time_ms, cpu);
    fclose(f);
}

// ─────────────────────────────────────────
// MAIN
// ─────────────────────────────────────────
int main() {

    printf("\n");
    printf("╔══════════════════════════════════╗\n");
    printf("║  Thread vs Process Analyzer      ║\n");
    printf("║  Array Size: 1,000,000 elements  ║\n");
    printf("╚══════════════════════════════════╝\n\n");

    // ── Allocate and fill array ──
    long long *array = malloc(ARRAY_SIZE * sizeof(long long));
    if (!array) {
        printf("Memory allocation failed!\n");
        return 1;
    }
    for (int i = 0; i < ARRAY_SIZE; i++)
        array[i] = i + 1;

    printf("✓ Array filled with 1 to 1,000,000\n\n");

    // ── Init CSV ──
    FILE *f = fopen(CSV_FILE, "w");
    if (f) {
        fprintf(f, "mode,array_size,time_ms,cpu_usage\n");
        fclose(f);
    }

    double t_start, t_end, elapsed;
    long   cpu_before, cpu_after;
    long long result;

    // ════════════════════════════════
    // TEST 1: THREADS
    // ════════════════════════════════
    printf("Running Thread test...\n");
    cpu_before = get_cpu_ticks();
    t_start    = get_time_ms();

    result     = run_threads(array);

    t_end      = get_time_ms();
    cpu_after  = get_cpu_ticks();

    double thread_time = t_end - t_start;
    double thread_cpu  = get_cpu_usage(cpu_before, cpu_after, thread_time);

    printf("✓ Thread done!\n\n");
    save_csv("thread", thread_time, thread_cpu);

    // ════════════════════════════════
    // TEST 2: PROCESSES
    // ════════════════════════════════

    // Refill array (threads may have modified it)
    for (int i = 0; i < ARRAY_SIZE; i++)
        array[i] = i + 1;

    printf("Running Process test...\n");
    cpu_before = get_cpu_ticks();
    t_start    = get_time_ms();

    result     = run_processes(array);

    t_end      = get_time_ms();
    cpu_after  = get_cpu_ticks();

    double process_time = t_end - t_start;
    double process_cpu  = get_cpu_usage(cpu_before, cpu_after, process_time);

    printf("✓ Process done!\n\n");
    save_csv("process", process_time, process_cpu);

    // ════════════════════════════════
    // SUMMARY TABLE
    // ════════════════════════════════
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║              SUMMARY TABLE                   ║\n");
    printf("╠══════════════════════════════════════════════╣\n");
    printf("║  %-10s  %-12s  %-12s          ║\n",
           "Mode", "Time (ms)", "CPU (%)");
    printf("╠══════════════════════════════════════════════╣\n");
    printf("║  %-10s  %-12.4f  %-12.2f          ║\n",
           "Thread", thread_time, thread_cpu);
    printf("║  %-10s  %-12.4f  %-12.2f          ║\n",
           "Process", process_time, process_cpu);
    printf("╠══════════════════════════════════════════════╣\n");

    // Speedup
    double speedup = process_time / thread_time;
    printf("║  Speedup: Thread is %.2fx faster             ║\n", speedup);
    printf("╚══════════════════════════════════════════════╝\n\n");

    printf("✓ Results saved to %s\n", CSV_FILE);
    printf("✓ Run: python3 visualizer.py\n\n");

    free(array);
    return 0;
}
