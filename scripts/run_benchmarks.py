"""
Runs all experiments and writes results/results.csv and results/profile.csv

Run from the project root:
    python scripts/run_benchmarks.py                 # full run
    python scripts/run_benchmarks.py --quick         # fast test run

Options:
    --sizes small medium large
    --counts 10 25 50 100
    --threads 1 2 4 
    --repeats 3        (median of N runs is reported)
"""
import argparse
import csv
import os
import statistics
import subprocess
import sys

SIZE_DIRS = {"small": "data/small", "medium": "data/medium", "large": "data/large"}


def exe(name):
    for cand in (f"build/{name}.exe", f"build/{name}"):
        if os.path.exists(cand):
            return cand
    sys.exit(f"Cannot find build/{name}. Build the project first (see README).")


def run(cmd):
    out = subprocess.run(cmd, capture_output=True, text=True)
    if out.returncode != 0:
        sys.exit(f"Command failed: {' '.join(cmd)}\n{out.stderr}")
    for line in out.stdout.splitlines():
        if line.startswith(("RESULT,", "PROFILE,")):
            return line.split(",")
    sys.exit(f"No result line from: {' '.join(cmd)}")


def median_run(cmd, repeats):
    times, checksum, width = [], None, None
    for _ in range(repeats):
        r = run(cmd)
        times.append(float(r[6]))
        checksum = r[8]
        width = r[4]
    return statistics.median(times), checksum, width


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sizes", nargs="+", default=["small", "medium", "large"])
    ap.add_argument("--counts", nargs="+", type=int, default=[10, 25, 50, 100])
    ap.add_argument("--threads", nargs="+", type=int, default=[1, 2, 4, 8])
    ap.add_argument("--repeats", type=int, default=3)
    ap.add_argument("--quick", action="store_true")
    a = ap.parse_args()

    if a.quick:
        a.sizes, a.counts, a.threads, a.repeats = ["small"], [10, 20], [1, 2, 4], 1

    seq, omp = exe("seq"), exe("omp")
    os.makedirs("results", exist_ok=True)

    rows = []
    for size in a.sizes:
        folder = SIZE_DIRS[size]
        if not os.path.isdir(folder):
            print(f"Skipping {size}: {folder} not found")
            continue
        for n in a.counts:
            t_seq, ck_seq, width = median_run([seq, folder, str(n)], a.repeats)
            print(f"[{size} {width}px] N={n:3d}  seq        {t_seq:8.3f} s")
            rows.append([size, width, n, "seq", 1, t_seq, 1.0, 1.0, "yes"])

            for t in a.threads:
                t_par, ck_par, _ = median_run([omp, folder, str(n), str(t)], a.repeats)
                ok = "yes" if ck_par == ck_seq else "NO"
                sp = t_seq / t_par
                eff = sp / t
                print(f"[{size} {width}px] N={n:3d}  omp T={t:<2d}  {t_par:8.3f} s  "
                      f"speedup {sp:5.2f}  eff {eff:5.2f}  correct={ok}")
                rows.append([size, width, n, "omp", t, t_par, sp, eff, ok])

    with open("results/results.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["size", "width", "images", "mode", "threads",
                    "time_s", "speedup", "efficiency", "correct"])
        w.writerows(rows)
    print("\nSaved results/results.csv")

    # ---- per-stage profile (sequential) ----
    prof = []
    for size in a.sizes:
        folder = SIZE_DIRS[size]
        if os.path.isdir(folder):
            r = run([seq, folder, "10", "--profile"])
            prof.append([size, r[1], r[3], r[4], r[5]])
    with open("results/profile.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["size", "width", "estimate_ms", "deblur_ms", "sharpen_ms"])
        w.writerows(prof)
    print("Saved results/profile.csv")


if __name__ == "__main__":
    main()
