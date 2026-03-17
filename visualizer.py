import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

# ─────────────────────────────────────────
# LOAD DATA
# ─────────────────────────────────────────
df = pd.read_csv("results.csv")
print("✓ Data loaded:")
print(df.to_string(index=False))

# ── Extract values ──
thread_time    = df[df["mode"]=="thread"]["time_ms"].values[0]
process_time   = df[df["mode"]=="process"]["time_ms"].values[0]
thread_cpu     = df[df["mode"]=="thread"]["cpu_usage"].values[0]
process_cpu    = df[df["mode"]=="process"]["cpu_usage"].values[0]
thread_vol     = df[df["mode"]=="thread"]["vol_switches"].values[0]
thread_nonvol  = df[df["mode"]=="thread"]["nonvol_switches"].values[0]
process_vol    = df[df["mode"]=="process"]["vol_switches"].values[0]
process_nonvol = df[df["mode"]=="process"]["nonvol_switches"].values[0]
thread_ctx     = df[df["mode"]=="thread"]["total_switches"].values[0]
process_ctx    = df[df["mode"]=="process"]["total_switches"].values[0]
speedup        = process_time / thread_time

modes  = ["Thread", "Process"]
colors = ["#2196F3", "#FF5722"]

# ─────────────────────────────────────────
# DASHBOARD — 2x2 grid
# ─────────────────────────────────────────
fig, axes = plt.subplots(2, 2, figsize=(14, 10))
fig.suptitle(
    "Thread vs Process Performance Dashboard\n"
    "(Array Size: 1,000,000)",
    fontsize=16, fontweight="bold")

# ════════════════════════════
# Chart 1: Execution Time
# ════════════════════════════
bars = axes[0][0].bar(modes,
                      [thread_time, process_time],
                      color=colors, alpha=0.85,
                      edgecolor="white", width=0.5)
axes[0][0].set_title("Execution Time",
                     fontsize=13, fontweight="bold")
axes[0][0].set_ylabel("Time (ms)", fontsize=11)
axes[0][0].set_facecolor("#f9f9f9")
axes[0][0].grid(axis="y", alpha=0.3)
for bar, val in zip(bars, [thread_time, process_time]):
    axes[0][0].text(
        bar.get_x() + bar.get_width()/2,
        bar.get_height() + 0.05,
        f"{val:.4f} ms",
        ha="center", fontsize=11, fontweight="bold")

# ════════════════════════════
# Chart 2: CPU Usage
# ════════════════════════════
bars2 = axes[0][1].bar(modes,
                       [thread_cpu, process_cpu],
                       color=colors, alpha=0.85,
                       edgecolor="white", width=0.5)
axes[0][1].set_title("CPU Usage",
                     fontsize=13, fontweight="bold")
axes[0][1].set_ylabel("CPU (%)", fontsize=11)
axes[0][1].set_facecolor("#f9f9f9")
axes[0][1].grid(axis="y", alpha=0.3)
for bar, val in zip(bars2, [thread_cpu, process_cpu]):
    axes[0][1].text(
        bar.get_x() + bar.get_width()/2,
        bar.get_height() + 1,
        f"{val:.2f}%",
        ha="center", fontsize=11, fontweight="bold")

# ════════════════════════════
# Chart 3: Context Switches
# (stacked bar — vol + nonvol)
# ════════════════════════════
x     = np.arange(len(modes))
width = 0.45

vol_vals    = [thread_vol,    process_vol]
nonvol_vals = [thread_nonvol, process_nonvol]

b1 = axes[1][0].bar(x, vol_vals, width,
                    label="Voluntary",
                    color=["#1565C0", "#BF360C"],
                    alpha=0.9)
b2 = axes[1][0].bar(x, nonvol_vals, width,
                    bottom=vol_vals,
                    label="Non-voluntary",
                    color=["#64B5F6", "#FF8A65"],
                    alpha=0.9)

axes[1][0].set_title("Context Switches (Stacked)",
                     fontsize=13, fontweight="bold")
axes[1][0].set_ylabel("Number of switches", fontsize=11)
axes[1][0].set_xticks(x)
axes[1][0].set_xticklabels(modes)
axes[1][0].legend(fontsize=10)
axes[1][0].set_facecolor("#f9f9f9")
axes[1][0].grid(axis="y", alpha=0.3)

# Total labels on top of stacked bars
for i, (v, nv) in enumerate(zip(vol_vals, nonvol_vals)):
    axes[1][0].text(i, v + nv + 0.1,
                    f"Total: {int(v+nv)}",
                    ha="center", fontsize=11,
                    fontweight="bold")

# ════════════════════════════
# Chart 4: Speedup Summary
# ════════════════════════════
categories  = ["Time\nSpeedup", "Ctx Switch\nRatio"]
ctx_ratio   = (process_ctx / thread_ctx
               if thread_ctx > 0 else 0)
values      = [speedup, ctx_ratio]
bar_colors  = ["#4CAF50", "#9C27B0"]

bars4 = axes[1][1].bar(categories, values,
                       color=bar_colors,
                       alpha=0.85,
                       edgecolor="white",
                       width=0.4)
axes[1][1].axhline(y=1, color="red",
                   linestyle="--", alpha=0.5,
                   label="Baseline (1x)")
axes[1][1].set_title("Thread Advantage",
                     fontsize=13, fontweight="bold")
axes[1][1].set_ylabel("Ratio (higher = thread wins more)",
                      fontsize=10)
axes[1][1].set_facecolor("#f9f9f9")
axes[1][1].grid(axis="y", alpha=0.3)
axes[1][1].legend(fontsize=10)

for bar, val in zip(bars4, values):
    axes[1][1].text(
        bar.get_x() + bar.get_width()/2,
        bar.get_height() + 0.02,
        f"{val:.2f}x",
        ha="center", fontsize=12,
        fontweight="bold")

plt.tight_layout()
plt.savefig("dashboard.png", dpi=150,
            bbox_inches="tight")
print("\n✓ Saved: dashboard.png")

# ─────────────────────────────────────────
# PRINT SUMMARY TABLE
# ─────────────────────────────────────────
print("\n" + "═"*60)
print("  COMPLETE RESULTS SUMMARY")
print("═"*60)
print(f"  {'Metric':<25} {'Thread':>12} {'Process':>12}")
print("─"*60)
print(f"  {'Time (ms)':<25} {thread_time:>12.4f} {process_time:>12.4f}")
print(f"  {'CPU Usage (%)':<25} {thread_cpu:>12.2f} {process_cpu:>12.2f}")
print(f"  {'Voluntary switches':<25} {thread_vol:>12} {process_vol:>12}")
print(f"  {'Non-voluntary switches':<25} {thread_nonvol:>12} {process_nonvol:>12}")
print(f"  {'Total ctx switches':<25} {thread_ctx:>12} {process_ctx:>12}")
print("─"*60)
print(f"  {'Time speedup':<25} {'Thread is':>12} {speedup:.2f}x faster")
print(f"  {'Switch ratio':<25} {'Process has':>10} {ctx_ratio:.1f}x more")
print("═"*60)

plt.show()
