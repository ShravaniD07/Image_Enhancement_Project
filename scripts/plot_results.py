"""
Makes graphs + a summary table from results/results.csv and results/profile.csv

Run from the project root:   python scripts/plot_results.py
Needs:  pip install matplotlib
"""
import csv
import os
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

os.makedirs("results", exist_ok=True)

rows = list(csv.DictReader(open("results/results.csv")))
for r in rows:
    r["images"] = int(r["images"])
    r["threads"] = int(r["threads"])
    r["time_s"] = float(r["time_s"])
    r["speedup"] = float(r["speedup"])
    r["efficiency"] = float(r["efficiency"])

sizes = []
for r in rows:
    if r["size"] not in sizes:
        sizes.append(r["size"])
width = {r["size"]: r["width"] for r in rows}
max_n = max(r["images"] for r in rows)
omp = [r for r in rows if r["mode"] == "omp"]
seq = {(r["size"], r["images"]): r["time_s"] for r in rows if r["mode"] == "seq"}
threads_all = sorted({r["threads"] for r in omp})


def label(s):
    return f"{s} ({width[s]}x{width[s]})"


# 1) Speedup vs threads (largest image count)
plt.figure(figsize=(6, 4.5))
for s in sizes:
    pts = sorted((r["threads"], r["speedup"]) for r in omp
                 if r["size"] == s and r["images"] == max_n)
    if pts:
        plt.plot(*zip(*pts), marker="o", label=label(s))
plt.plot(threads_all, threads_all, "k--", label="ideal")
plt.xlabel("Threads"); plt.ylabel("Speedup")
plt.title(f"Speedup vs threads ({max_n} images)")
plt.grid(alpha=.3); plt.legend(); plt.tight_layout()
plt.savefig("results/speedup.png", dpi=150); plt.close()

# 2) Efficiency vs threads
plt.figure(figsize=(6, 4.5))
for s in sizes:
    pts = sorted((r["threads"], r["efficiency"]) for r in omp
                 if r["size"] == s and r["images"] == max_n)
    if pts:
        plt.plot(*zip(*pts), marker="o", label=label(s))
plt.axhline(1.0, color="k", linestyle="--", label="ideal")
plt.xlabel("Threads"); plt.ylabel("Efficiency (speedup / threads)")
plt.title(f"Parallel efficiency ({max_n} images)")
plt.ylim(0, 1.2); plt.grid(alpha=.3); plt.legend(); plt.tight_layout()
plt.savefig("results/efficiency.png", dpi=150); plt.close()

# 3) Time vs number of images (sequential vs OpenMP with max threads)
tmax = max(threads_all)
fig, axes = plt.subplots(1, len(sizes), figsize=(5 * len(sizes), 4), squeeze=False)
for ax, s in zip(axes[0], sizes):
    ns = sorted({n for (sz, n) in seq if sz == s})
    ax.plot(ns, [seq[(s, n)] for n in ns], marker="o", label="sequential")
    par = {r["images"]: r["time_s"] for r in omp
           if r["size"] == s and r["threads"] == tmax}
    ax.plot(sorted(par), [par[n] for n in sorted(par)], marker="s",
            label=f"OpenMP ({tmax} threads)")
    ax.set_title(label(s)); ax.set_xlabel("Number of images")
    ax.set_ylabel("Time (s)"); ax.grid(alpha=.3); ax.legend()
plt.tight_layout(); plt.savefig("results/time_vs_images.png", dpi=150); plt.close()

# 4) Time per image vs image size (sequential, largest count)
plt.figure(figsize=(6, 4.5))
xs = [int(width[s]) ** 2 / 1e6 for s in sizes]
ys = [seq[(s, max_n)] / max_n for s in sizes if (s, max_n) in seq]
if len(ys) == len(xs):
    plt.plot(xs, ys, marker="o")
    for x, y, s in zip(xs, ys, sizes):
        plt.annotate(label(s), (x, y), textcoords="offset points", xytext=(5, -12))
    plt.xlabel("Image size (megapixels)"); plt.ylabel("Sequential time per image (s)")
    plt.title("Effect of image size"); plt.grid(alpha=.3); plt.tight_layout()
    plt.savefig("results/time_vs_size.png", dpi=150)
plt.close()

# 5) Stage profile
if os.path.exists("results/profile.csv"):
    prof = list(csv.DictReader(open("results/profile.csv")))
    if prof:
        names = [f"{p['size']}\n({p['width']}px)" for p in prof]
        est = [float(p["estimate_ms"]) for p in prof]
        deb = [float(p["deblur_ms"]) for p in prof]
        shp = [float(p["sharpen_ms"]) for p in prof]
        plt.figure(figsize=(6, 4.5))
        plt.bar(names, est, label="radius estimation")
        plt.bar(names, deb, bottom=est, label="Wiener deblur")
        plt.bar(names, shp, bottom=[a + b for a, b in zip(est, deb)], label="sharpen")
        plt.ylabel("ms per image (sequential)")
        plt.title("Where the time goes"); plt.legend(); plt.tight_layout()
        plt.savefig("results/stage_profile.png", dpi=150); plt.close()

# ---- summary table (markdown, paste into the report) ----
with open("results/summary.md", "w") as f:
    f.write("| Size | Images | Threads | Time (s) | Speedup | Efficiency | Correct |\n")
    f.write("|---|---|---|---|---|---|---|\n")
    for r in rows:
        f.write(f"| {r['size']} ({r['width']}px) | {r['images']} | "
                f"{'1 (seq)' if r['mode'] == 'seq' else r['threads']} | "
                f"{r['time_s']:.3f} | {r['speedup']:.2f} | "
                f"{r['efficiency']:.2f} | {r['correct']} |\n")

print("Saved: speedup.png, efficiency.png, time_vs_images.png, "
      "time_vs_size.png, stage_profile.png, summary.md  (in results/)")
