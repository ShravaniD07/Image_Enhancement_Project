# Parallel Image Enhancement (Q5)

Applies two computationally heavy enhancement operations to a large
collection of images, first sequentially and then in parallel with OpenMP,
and measures how performance changes with number of threads, number of
images and image size.

**Operations (per image, fully automatic)**
1. Blur-radius estimation (FFT based) + **Wiener deconvolution** (removes defocus blur, Y channel only)
2. **High-boost sharpening / unsharp mask** (Y channel only)

**Parallel strategy:** OpenMP data parallelism across images
(`#pragma omp parallel for schedule(dynamic,1)`). Images are independent,
so there is no communication and no locking.

## Folder layout
```
Parallel_Image_Enhancement/
├── src/        image_enhancement.{h,cpp}  common.h  sequential.cpp  parallel_omp.cpp
├── scripts/    make_sizes.py  run_benchmarks.py  plot_results.py
├── data/       small/ (512px)  medium/ (1024px)  large/ (2048px)
├── build/      executables (seq, omp)
├── results/    results.csv  profile.csv  summary.md  *.png graphs
├── output/     enhanced images (optional)
└── docs/       report_outline.md  LLM_usage_log.md
```

## Step 1 - Install (MSYS2 UCRT64 terminal)
```
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-opencv \
  mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-python-matplotlib \
  mingw-w64-ucrt-x86_64-python-opencv
```

## Step 2 - Build (from the project root)
Option A - plain g++ (simplest):
```
mkdir -p build
g++ -O2 -std=c++17 src/image_enhancement.cpp src/sequential.cpp -Isrc -o build/seq $(pkg-config --cflags --libs opencv4)
g++ -O2 -std=c++17 -fopenmp src/image_enhancement.cpp src/parallel_omp.cpp -Isrc -o build/omp $(pkg-config --cflags --libs opencv4)
```
Option B - CMake:
```
cmake -S . -B build_cmake -G "MinGW Makefiles"
cmake --build build_cmake
```
(CMake writes the executables into `build/` as well.)

## Step 3 - Dataset
1. Unzip `blurred_dataset_100.zip` and copy the 100 PNGs into `data/small/`.
2. Create the larger versions:
```
python scripts/make_sizes.py
```
This fills `data/medium/` (1024px) and `data/large/` (2048px).

## Step 4 - Try it once
```
./build/seq data/small 20
./build/omp data/small 20 4
```
Both print one line: `RESULT,mode,threads,images,width,height,compute_s,load_s,checksum`.
**The checksum must be identical for seq and omp** - that is the correctness check.

Save the enhanced images (not timed):
```
./build/omp data/small 10 4 --save output/small
```

## Step 5 - Run all experiments
```
python scripts/run_benchmarks.py --quick      # 1-minute test
python scripts/run_benchmarks.py              # full run (can take 20-30 min)
```
Useful options: `--sizes small medium`, `--counts 10 25 50`, `--threads 1 2 4 8`, `--repeats 3`.
Use thread counts up to your number of logical cores. Close other programs while it runs.
If memory is low, drop `--sizes large` or use fewer images for it.

## Step 6 - Graphs and tables
```
python scripts/plot_results.py
```
Creates in `results/`: `speedup.png`, `efficiency.png`, `time_vs_images.png`,
`time_vs_size.png`, `stage_profile.png`, and `summary.md` (table to paste into the report).

## Notes
- Timing covers only computation. Image loading is measured separately (`load_s`) and is sequential.
- `cv::setNumThreads(1)` keeps OpenCV single-threaded so OpenMP alone controls parallelism.
- Each reported time is the median of `--repeats` runs.
