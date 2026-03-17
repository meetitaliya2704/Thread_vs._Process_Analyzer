import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# ─────────────────────────────────────────
# LOAD DATA
# ─────────────────────────────────────────
df = pd.read_csv("results.csv")
print("✓ Data loaded:")
print(df.to_string(index=False))

# Extract values
thread_time  = df[df["mode"] == "thread"]["time_ms"].values[0]
process_time = df[df["mode"] == "process"]["time_ms"].values[0]
thread_cpu   = df[df["mode"] == "thread"]["cpu_usage"].values[0]
process_cpu  = df[df["mode"] == "process"]["cpu_usage"].values[0]
speedup      = process_time / thread_time

modes  = ["Thread", "Process"]
times  = [thread_time, process_time]
cpus   = [thread_cpu, process_cpu]
colors = ["#2196F3", "#FF5722"]

# ─────────────────────────────────────────
# DASHBOARD — 3 charts in one figure
# ─────────────────────────────────────────
fig, axes = plt.subplots(1, 3, figsize=(15, 6))
fig.suptitle(
    "Thread vs Process Performance\n(Array Size: 1,000,000)",
    fontsize=16, fontweight="bold")

# ── Chart 1: Execution Time ──
bars = axes[0].bar(modes, times, color=colors,
                   alpha=0.85, edgecolor="white", width=0.5)
axes[0].set_title("Execution Time", fontsize=13, fontweight="bold")
axes[0].set_ylabel("Time (ms)", fontsize=11)
axes[0].set_facecolor("#f9f9f9")
axes[0].grid(axis="y", alpha=0.3)
for bar, val in zip(bars, times):
    axes[0].text(bar.get_x() + bar.get_width()/2,
                 bar.get_height() + 0.05,
                 f"{val:.4f} ms",
                 ha="center", fontsize=11, fontweight="bold")

# ── Chart 2: CPU Usage ──
bars2 = axes[1].bar(modes, cpus, color=colors,
                    alpha=0.85, edgecolor="white", width=0.5)
axes[1].set_title("CPU Usage", fontsize=13, fontweight="bold")
axes[1].set_ylabel("CPU (%)", fontsize=11)
axes[1].set_facecolor("#f9f9f9")
axes[1].grid(axis="y", alpha=0.3)
for bar, val in zip(bars2, cpus):
    axes[1].text(bar.get_x() + bar.get_width()/2,
                 bar.get_height() + 1,
                 f"{val:.2f}%",
                 ha="center", fontsize=11, fontweight="bold")

# ── Chart 3: Speedup ──
axes[2].bar(["Speedup"], [speedup],
            color="#4CAF50", alpha=0.85,
            edgecolor="white", width=0.3)
axes[2].axhline(y=1, color="red",
                linestyle="--", alpha=0.6,
                label="Baseline (1x)")
axes[2].set_title("Thread Speedup vs Process",
                  fontsize=13, fontweight="bold")
axes[2].set_ylabel("Speedup (x times)", fontsize=11)
axes[2].set_facecolor("#f9f9f9")
axes[2].grid(axis="y", alpha=0.3)
axes[2].legend(fontsize=10)
axes[2].text(0, speedup + 0.02,
             f"{speedup:.2f}x faster",
             ha="center", fontsize=12, fontweight="bold",
             color="#2E7D32")

plt.tight_layout()
plt.savefig("dashboard.png", dpi=150, bbox_inches="tight")
print("\n✓ Saved: dashboard.png")

# ─────────────────────────────────────────
# PRINT SUMMARY
# ─────────────────────────────────────────
print("\n" + "═"*45)
print("  RESULTS SUMMARY")
print("═"*45)
print(f"  Thread  → {thread_time:.4f} ms | CPU: {thread_cpu:.2f}%")
print(f"  Process → {process_time:.4f} ms | CPU: {process_cpu:.2f}%")
print(f"  Speedup → Thread is {speedup:.2f}x faster")
print("═"*45)

plt.show()
