markdown
# Parallel Image Enhancement 

Applies two computationally heavy enhancement operations to a large collection of images, first sequentially and then in parallel with OpenMP, and measures how performance changes with number of threads, number of images, and image size.

**Operations (per image, fully automatic)**
1. Blur-radius estimation (FFT based) + **Wiener deconvolution** (removes defocus blur, Y channel only)
2. **High-boost sharpening / unsharp mask** (Y channel only)

**Parallel strategy:** OpenMP data parallelism across images (`#pragma omp parallel for schedule(dynamic,1)`). Images are independent, so there is no communication and no locking.

---

## 📁 Folder Layout
Use code with caution.
Parallel_Image_Enhancement/
├── src/        image_enhancement.{h,cpp}  common.h  sequential.cpp  parallel_omp.cpp
├── scripts/    make_sizes.py  run_benchmarks.py  plot_results.py
├── data/       small/ (512px)  medium/ (1024px)  large/ (2048px) 
├── build/      executables (seq, omp)
├── results/    results.csv  profile.csv  summary.md  *.png graphs
├── output/     enhanced images (optional)
└── docs/       report_outline.md  LLM_usage_log.md

---

## 🛠️ Step 1 - Environment Setup (OS Specific)

Choose the environment setup blocks matching your specific operating system target:

### 🪟 Windows (MSYS2 UCRT64 Terminal)
Launch your **MSYS2 UCRT64** environment and run:
```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-opencv \
  mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-python-matplotlib \
  mingw-w64-ucrt-x86_64-python-opencv
```

### 🐧 Linux (Ubuntu / Debian Terminal)
Update your system packages and install GCC, OpenMP, CMake, OpenCV development flags, and Python tooling:
```bash
sudo apt update
sudo apt install -y build-essential libomp-dev cmake pkg-config \
  libopencv-dev python3 python3-pip python3-matplotlib python3-opencv
```

### 🍏 macOS (Homebrew Terminal)
*Note: The native Apple Clang compiler does not inherently support OpenMP. You must use LLVM GCC or Homebrew's LLVM configuration.*
```bash
# Install dependencies via Homebrew
brew install gcc cmake opencv pkg-config python matplotlib

# Verify your specific Homebrew GCC version to use for Step 2 (e.g., gcc-13, gcc-14)
gcc-14 --version
```

---

## 🏗️ Step 2 - Build Configurations

Choose either **Option A (Plain CLI Compiling)** or **Option B (Recommended Multi-Platform CMake Engine)**. Execute commands directly from your project root.

### Option A: Plain CLI Compilation

#### Windows (MSYS2 UCRT64) / Linux
```bash
mkdir -p build
g++ -O2 -std=c++17 src/image_enhancement.cpp src/sequential.cpp -Isrc -o build/seq $(pkg-config --cflags --libs opencv4)
g++ -O2 -std=c++17 -fopenmp src/image_enhancement.cpp src/parallel_omp.cpp -Isrc -o build/omp $(pkg-config --cflags --libs opencv4)
```

#### macOS (Using Homebrew GCC to guarantee OpenMP support)
*Replace `g++-14` with the exact version installed via Homebrew on your system.*
```bash
mkdir -p build
g++-14 -O2 -std=c++17 src/image_enhancement.cpp src/sequential.cpp -Isrc -o build/seq $(pkg-config --cflags --libs opencv4)
g++-14 -O2 -std=c++17 -fopenmp src/image_enhancement.cpp src/parallel_omp.cpp -Isrc -o build/omp $(pkg-config --cflags --libs opencv4)
```

---

### Option B: CMake (Unified Cross-Platform Matrix)
Using CMake guarantees proper platform-specific linking across paths and extensions automatically.

#### Windows (MSYS2 UCRT64)
```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

#### Linux / macOS
```bash
# On macOS, force CMake to target the Homebrew GCC compiler for OpenMP runtime usage
# Example: -DCMAKE_C_COMPILER=gcc-14 -DCMAKE_CXX_COMPILER=g++-14

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## 📂 Step 3 - Dataset Management

1. Unzip `blurred_dataset_100.zip` and copy the 100 benchmark images into `data/small/`.
2. Generate scaled resolution suites using the automated Python configuration hook:
```bash
# Use 'python' or 'python3' based on system alias configurations
python3 scripts/make_sizes.py
```
This routine structures multi-tiered testing arrays within `data/medium/` (1024px) and `data/large/` (2048px).

---

## 🏃 Step 4 - Basic Execution Check

Run a short test run to confirm execution behaviors and check pipeline precision metrics. 

#### Windows
```bash
./build/seq.exe data/small 20
./build/omp.exe data/small 20 4
```

#### Linux / macOS
```bash
./build/seq data/small 20
./build/omp data/small 20 4
```

**Output Diagnostics:** Both binaries output a structured string tracking runtime data metrics:  
`RESULT,mode,threads,images,width,height,compute_s,load_s,checksum`

⚠️ **Correctness Invariance Check:** The generated `checksum` hash value **must evaluate identically** across both `seq` and `omp` run signatures to guarantee deterministic loop execution and verify zero race conditions.

To save image processing arrays to disk without benchmarking file system interference, pass the optional `--save` argument:
```bash
./build/omp data/small 10 4 --save output/small
```

---

## 📊 Step 5 - Comprehensive Benchmarking Execution

Execute your testing matrix workflows using the standard parameter flags:

```bash
# Quick Execution Check (~1 Minute runtime verification)
python3 scripts/run_benchmarks.py --quick      

# Comprehensive Evaluation Matrix Run (20 - 30 Minutes runtime overhead)
python3 scripts/run_benchmarks.py              
```

### Parameter Performance Customization Options
* Add `--sizes small medium` to limit memory usage on target systems.
* Use `--counts 10 25 50` to throttle baseline dataset processing array volumes.
* Set thread topologies via `--threads 1 2 4 8` matching your maximum logical system cores.
* Adjust validation iterations with `--repeats 3` to clean ambient timing deviations.

---

## 📈 Step 6 - Analytics Visualization and Tabulation

Process the performance results matrix output (`results.csv`) and output structural tracking charts:

```bash
python3 scripts/plot_results.py
```

The script populates files down to your `/results` pathway destination:
* `speedup.png` / `efficiency.png` — Scalability tracing relative to core availability.
* `time_vs_images.png` / `time_vs_size.png` — Compute overhead trends relative to processing scaling.
* `stage_profile.png` — Visual overhead footprint tracing of pipeline algorithms.
* `summary.md` — Auto-generated layout tables formatted for academic reports.

---

## 📝 Performance and System Engineering Notes
* **Isolated Timings:** Performance execution tracks pure compute workload algorithms. Thread load overhead configurations (`load_s`) operate outside timing loops.
* **Concurrency Protection:** The application configures `cv::setNumThreads(1)` natively to suppress implicit OpenCV thread pooling. Parallel workloads scale exclusively through your explicit OpenMP configuration parameters.
* **Data Cleansing:** System scheduling discrepancies are filtered by calculating the structural statistical median value across the active `--repeats` parameters array.

---

## 🤖 LLM Usage & Academic Integrity

This project utilized Large Language Models (LLMs) to accelerate prototyping, explore initial algorithm selections, and design benchmarking pipelines. In alignment with academic honesty and engineering practices, all AI interactions were rigorously validated, verified, and audited.

* **Code Verification:** All parallel implementation steps suggested by AI were refactored to prevent race conditions and verified using custom `pixelSum` checksum evaluations.
* **Algorithmic Audits:** Discovered and corrected critical flaws in AI-suggested algorithms and parameters (e.g., swapping out Richardson-Lucy for Wiener Deconvolution, and tuning the Wiener constant K from an unstable default to an empirically derived optimal value).
* **Detailed Logs:** A full, itemized record tracing every prompt, its modification history, and validation outcomes can be reviewed in our comprehensive [LLM Prompt Documentation Log](docs/LLM_usage_log.md).
Would you like me to write the complete CMakeLists.txt script next so that Option B works perfectly out of the box on Windows, Linux, and macOS?
