# Thread vs Process Performance Analyzer

![License](https://img.shields.io/badge/License-MIT-blue.svg)
![Language](https://img.shields.io/badge/Language-C-orange.svg)
![Python](https://img.shields.io/badge/Python-3.10+-green.svg)
![Platform](https://img.shields.io/badge/Platform-Linux-yellow.svg)

A performance benchmarking tool written in **C** with **Python
visualization** that compares the execution efficiency of POSIX
threads vs forked processes on array sum operations — measuring
execution time, CPU usage, and context switches.

---

## Table of Contents

- [Project Structure](#project-structure)
- [System Requirements](#system-requirements)
- [Installation](#installation)
- [Usage](#usage)
- [How It Works](#how-it-works)
- [Metrics Explained](#metrics-explained)
- [Understanding the Results](#understanding-the-results)
- [Configuration](#configuration)
- [Key Concepts](#key-concepts)
- [Technologies Used](#technologies-used)
- [License](#license)

---

## Project Structure
```
simple_analyzer/
├── analyzer.c        → Core C benchmark (threads + processes)
├── visualizer.py     → Python dashboard and graphs
├── requirements.txt  → Python dependencies
├── results.csv       → Generated benchmark output
├── dashboard.png     → Generated performance graph
├── LICENSE           → MIT License
└── README.md         → This file
```

---

## System Requirements

| Requirement | Details                        |
|-------------|--------------------------------|
| OS          | Linux (Ubuntu 20.04+) or WSL2  |
| Compiler    | GCC 11+                        |
| Python      | 3.10+                          |
| CPU Cores   | 4 recommended                  |
| RAM         | 512MB minimum                  |
| Disk Space  | 100MB free                     |

---

## Installation

### Step 1 — Get the project
```bash
mkdir ~/simple_analyzer
cd ~/simple_analyzer
```

### Step 2 — Install Python dependencies
```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

### Step 3 — Compile the C program
```bash
gcc analyzer.c -pthread -o analyzer
```

---

## Usage

### Step 1 — Run the benchmark
```bash
./analyzer
```

**Expected output:**
```
╔══════════════════════════════════════╗
║   Thread vs Process Analyzer         ║
║   Array Size : 1,000,000 elements    ║
║   Metrics    : Time, CPU, Ctx Switch ║
╚══════════════════════════════════════╝

✓ Array filled: 1 to 1,000,000

Running Thread test...
✓ Thread done!

Running Process test...
✓ Process done!

╔══════════════════════════════════════════════════════════╗
║                    SUMMARY TABLE                         ║
╠══════════════════════════════════════════════════════════╣
║  Mode       Time(ms)   CPU(%)     Ctx Total  Vol/NonVol  ║
╠══════════════════════════════════════════════════════════╣
║  Thread     3.8218     523.32     4          4/0         ║
║  Process    5.3344     0.00       18         12/6        ║
╠══════════════════════════════════════════════════════════╣
║  Time Speedup   : Thread is 1.40x faster                 ║
║  Switch Ratio   : Process has 4x more switches           ║
╚══════════════════════════════════════════════════════════╝

✓ Results saved to results.csv
✓ Run: python3 visualizer.py
```

### Step 2 — Generate visualization
```bash
source venv/bin/activate
python3 visualizer.py
```

### Step 3 — View the dashboard
```bash
eog dashboard.png
```

---

## How It Works

### Threads (pthreads)
```
Array [1 ... 1,000,000]
            ↓ split into 4 equal chunks
Thread 1 → [1       ... 250,000]    Core 0
Thread 2 → [250,001 ... 500,000]    Core 1
Thread 3 → [500,001 ... 750,000]    Core 2
Thread 4 → [750,001 ... 1,000,000]  Core 3
            ↓ all run simultaneously
            ↓ results combined by main thread
          Final Sum
```

- Created with `pthread_create()`
- Share the same memory space
- Results collected via shared struct
- Waited on with `pthread_join()`

### Processes (fork)
```
Parent Process
├── fork() → Child 0 → sums chunk 0 → writes to pipe[0]
├── fork() → Child 1 → sums chunk 1 → writes to pipe[1]
├── fork() → Child 2 → sums chunk 2 → writes to pipe[2]
└── fork() → Child 3 → sums chunk 3 → writes to pipe[3]
Parent reads all 4 pipes → combines results → Final Sum
```

- Created with `fork()`
- Each process gets its own memory copy
- Results sent back to parent via `pipe()`
- Cleaned up with `wait()`

---

## Metrics Explained

### 1. Execution Time
Measured using `clock_gettime(CLOCK_MONOTONIC)` for
nanosecond precision. Reported in milliseconds.

### 2. CPU Usage
Read from `/proc/self/stat` fields 14 (utime)
and 15 (stime). Calculated as:
```
CPU% = (cpu_ticks_used / elapsed_time) × 100
```

Values above 100% mean multiple cores are being used.

### 3. Context Switches
Read from `/proc/self/status`:
```
voluntary_ctxt_switches    → process chose to yield CPU
nonvoluntary_ctxt_switches → OS forced a context switch
```

---

## Understanding the Results

### Why Threads are Faster

| Reason           | Explanation                        |
|------------------|------------------------------------|
| Shared memory    | No data copying needed             |
| Less overhead    | No fork() call required            |
| Direct results   | No pipe communication needed       |
| Cache friendly   | All threads share same address space|

### Why Processes have More Context Switches

| Reason             | Explanation                          |
|--------------------|--------------------------------------|
| pipe read() blocks | Parent waits for each child result   |
| fork() overhead    | OS scheduler activity on creation    |
| Separate memory    | More kernel involvement              |
| IPC required       | pipe read/write causes voluntary yields|

### Sample Results
```
Metric                     Thread        Process
─────────────────────────────────────────────────
Time (ms)                  3.8218        5.3344
CPU Usage (%)              523.32        0.00
Voluntary switches         4             12
Non-voluntary switches     0             6
Total context switches     4             18
─────────────────────────────────────────────────
Time speedup               Thread is 1.40x faster
Switch ratio               Process has 4x more
```

---

## Output Files

| File            | Description                     |
|-----------------|---------------------------------|
| `results.csv`   | Raw benchmark data              |
| `dashboard.png` | 2x2 performance chart           |

### CSV Format
```
mode,array_size,time_ms,cpu_usage,vol_switches,
nonvol_switches,total_switches

thread,1000000,3.8218,523.32,4,0,4
process,1000000,5.3344,0.00,12,6,18
```

---

## Configuration

To change array size or worker count edit these
constants at the top of `analyzer.c`:
```c
#define ARRAY_SIZE  1000000   // Number of elements
#define NUM_THREADS 4         // Number of threads
#define NUM_PROCS   4         // Number of processes
```

Then recompile:
```bash
gcc analyzer.c -pthread -o analyzer
```

---

## Key Concepts

| Concept               | Used In                        |
|-----------------------|--------------------------------|
| `pthreads`            | Thread creation and joining    |
| `fork()`              | Process creation               |
| `pipe()`              | Inter-process communication    |
| `clock_gettime()`     | Nanosecond precision timing    |
| `/proc/self/stat`     | CPU usage measurement          |
| `/proc/self/status`   | Context switch counting        |
| `wait()`              | Child process cleanup          |
| `pthread_join()`      | Thread synchronization         |

---

## Technologies Used

| Technology  | Purpose                    |
|-------------|----------------------------|
| C (GCC)     | Core benchmarking engine   |
| pthreads    | Thread implementation      |
| fork/pipe   | Process implementation     |
| /proc fs    | System metrics collection  |
| Python 3    | Visualization layer        |
| pandas      | CSV data processing        |
| matplotlib  | Graph generation           |
| numpy       | Numerical calculations     |

---

## License

MIT License — Copyright (c) 2026 Meet Italiya

This project is free to use, modify, and distribute
with proper attribution.
See [LICENSE](LICENSE) file for full details.
