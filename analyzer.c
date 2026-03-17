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
// CONTEXT SWITCH structure
// ─────────────────────────────────────────
typedef struct {
    long voluntary;
    long nonvoluntary;
    long total;
} CtxSwitch;

// ─────────────────────────────────────────
// THREAD WORKER
// ─────────────────────────────────────────
void *thread_sum(void *arg) {
    ThreadData *d = (ThreadData *)arg;
    d->result = 0;
    for (int i = d->start; i < d->end; i++)
        d->result += d->array[i];
    pthread_exit(NULL);
}

// ─────────────────────────────────────────
// RUN THREADS
// ─────────────────────────────────────────
long long run_threads(long long *array) {
    pthread_t  threads[NUM_THREADS];
    ThreadData data[NUM_THREADS];
    int chunk = ARRAY_SIZE / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        data[i].array  = array;
        data[i].start  = i * chunk;
        data[i].end    = (i == NUM_THREADS-1)
                         ? ARRAY_SIZE : (i+1)*chunk;
        data[i].result = 0;
        pthread_create(&threads[i], NULL,
                       thread_sum, &data[i]);
    }

    long long total = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total += data[i].result;
    }
    return total;
}

// ─────────────────────────────────────────
// RUN PROCESSES
// ─────────────────────────────────────────
long long run_processes(long long *array) {
    int pipes[NUM_PROCS][2];
    int chunk = ARRAY_SIZE / NUM_PROCS;

    for (int i = 0; i < NUM_PROCS; i++)
        pipe(pipes[i]);

    for (int i = 0; i < NUM_PROCS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            int start     = i * chunk;
            int end       = (i == NUM_PROCS-1)
                            ? ARRAY_SIZE : (i+1)*chunk;
            long long sum = 0;
            for (int j = start; j < end; j++)
                sum += array[j];
            close(pipes[i][0]);
            write(pipes[i][1], &sum, sizeof(long long));
            close(pipes[i][1]);
            exit(0);
        }
    }

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
// GET CPU TICKS
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
double get_cpu_usage(long before, long after,
                     double elapsed_ms) {
    long   ticks      = after - before;
    long   tps        = sysconf(_SC_CLK_TCK);
    double cpu_ms     = (ticks * 1000.0) / tps;
    if (elapsed_ms <= 0) return 0.0;
    return (cpu_ms / elapsed_ms) * 100.0;
}

// ─────────────────────────────────────────
// READ CONTEXT SWITCHES
// from /proc/self/status
// ─────────────────────────────────────────
void read_ctx_switches(CtxSwitch *ctx) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) {
        ctx->voluntary    = 0;
        ctx->nonvoluntary = 0;
        ctx->total        = 0;
        return;
    }
    char line[256];
    ctx->voluntary    = 0;
    ctx->nonvoluntary = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line,
            "voluntary_ctxt_switches:", 24) == 0)
            sscanf(line,
                "voluntary_ctxt_switches: %ld",
                &ctx->voluntary);
        if (strncmp(line,
            "nonvoluntary_ctxt_switches:", 27) == 0)
            sscanf(line,
                "nonvoluntary_ctxt_switches: %ld",
                &ctx->nonvoluntary);
    }
    fclose(f);
    ctx->total = ctx->voluntary + ctx->nonvoluntary;
}

// ─────────────────────────────────────────
// SAVE TO CSV
// ─────────────────────────────────────────
void save_csv(char *mode, double time_ms, double cpu,
              long vol, long nonvol, long total_ctx) {
    FILE *f = fopen(CSV_FILE, "a");
    if (!f) return;
    fprintf(f, "%s,%d,%.4f,%.2f,%ld,%ld,%ld\n",
            mode, ARRAY_SIZE,
            time_ms, cpu,
            vol, nonvol, total_ctx);
    fclose(f);
}

// ─────────────────────────────────────────
// MAIN
// ─────────────────────────────────────────
int main() {

    printf("\n");
    printf("╔══════════════════════════════════════╗\n");
    printf("║   Thread vs Process Analyzer         ║\n");
    printf("║   Array Size : 1,000,000 elements    ║\n");
    printf("║   Metrics    : Time, CPU, Ctx Switch ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    // ── Allocate and fill array ──
    long long *array = malloc(ARRAY_SIZE * sizeof(long long));
    if (!array) {
        printf("Memory allocation failed!\n");
        return 1;
    }
    for (int i = 0; i < ARRAY_SIZE; i++)
        array[i] = i + 1;
    printf("✓ Array filled: 1 to 1,000,000\n\n");

    // ── Init CSV ──
    FILE *f = fopen(CSV_FILE, "w");
    if (f) {
        fprintf(f, "mode,array_size,time_ms,cpu_usage,"
                   "vol_switches,nonvol_switches,"
                   "total_switches\n");
        fclose(f);
    }

    double    t_start, t_end;
    long      cpu_before, cpu_after;
    CtxSwitch ctx_before, ctx_after;
    long long result;

    // ════════════════════════════════
    // TEST 1: THREADS
    // ════════════════════════════════
    printf("Running Thread test...\n");
    cpu_before = get_cpu_ticks();
    read_ctx_switches(&ctx_before);
    t_start    = get_time_ms();

    result     = run_threads(array);

    t_end      = get_time_ms();
    cpu_after  = get_cpu_ticks();
    read_ctx_switches(&ctx_after);

    double thread_time     = t_end - t_start;
    double thread_cpu      = get_cpu_usage(
                                cpu_before, cpu_after,
                                thread_time);
    long thread_vol        = ctx_after.voluntary
                           - ctx_before.voluntary;
    long thread_nonvol     = ctx_after.nonvoluntary
                           - ctx_before.nonvoluntary;
    long thread_ctx_total  = ctx_after.total
                           - ctx_before.total;

    printf("✓ Thread done!\n\n");
    save_csv("thread", thread_time, thread_cpu,
             thread_vol, thread_nonvol,
             thread_ctx_total);

    // ════════════════════════════════
    // TEST 2: PROCESSES
    // ════════════════════════════════
    for (int i = 0; i < ARRAY_SIZE; i++)
        array[i] = i + 1;

    printf("Running Process test...\n");
    cpu_before = get_cpu_ticks();
    read_ctx_switches(&ctx_before);
    t_start    = get_time_ms();

    result     = run_processes(array);

    t_end      = get_time_ms();
    cpu_after  = get_cpu_ticks();
    read_ctx_switches(&ctx_after);

    double process_time     = t_end - t_start;
    double process_cpu      = get_cpu_usage(
                                 cpu_before, cpu_after,
                                 process_time);
    long process_vol        = ctx_after.voluntary
                            - ctx_before.voluntary;
    long process_nonvol     = ctx_after.nonvoluntary
                            - ctx_before.nonvoluntary;
    long process_ctx_total  = ctx_after.total
                            - ctx_before.total;

    printf("✓ Process done!\n\n");
    save_csv("process", process_time, process_cpu,
             process_vol, process_nonvol,
             process_ctx_total);

    // ════════════════════════════════
    // SUMMARY TABLE
    // ════════════════════════════════
    double speedup    = process_time / thread_time;
    long   ctx_ratio  = (thread_ctx_total > 0)
                        ? process_ctx_total / thread_ctx_total
                        : 0;

    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║                    SUMMARY TABLE                         ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  %-10s %-10s %-10s %-10s %-12s  ║\n",
           "Mode", "Time(ms)", "CPU(%)",
           "Ctx Total", "Vol/NonVol");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  %-10s %-10.4f %-10.2f %-10ld %ld/%-10ld  ║\n",
           "Thread", thread_time, thread_cpu,
           thread_ctx_total,
           thread_vol, thread_nonvol);
    printf("║  %-10s %-10.4f %-10.2f %-10ld %ld/%-10ld  ║\n",
           "Process", process_time, process_cpu,
           process_ctx_total,
           process_vol, process_nonvol);
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Time Speedup   : Thread is %.2fx faster                 ║\n",
           speedup);
    printf("║  Switch Ratio   : Process has %ldx more switches         ║\n",
           ctx_ratio);
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    printf("✓ Results saved to %s\n", CSV_FILE);
    printf("✓ Run: python3 visualizer.py\n\n");

    free(array);
    return 0;
}
