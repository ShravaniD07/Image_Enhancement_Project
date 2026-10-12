# Report outline (fill in with YOUR results and words)

## 1. Problem definition
Enhance a large collection of images with computationally intensive operations
and study performance as the number and size of images grow.

## 2. Operations
- Wiener deconvolution: F = G * conj(H) / (|H|^2 + K), H = FFT of a disk PSF, K = 0.02. Y channel only.
- Blur-radius estimation: a disk blur puts a dip in the spectrum at f = 0.61 / radius; the radius with the deepest dip is chosen.
- High-boost sharpening: out = in + 0.4 * (in - GaussianBlur(in, sigma=1)).

## 3. Sequential design
Loop over images, run the pipeline on each (src/sequential.cpp).

## 4. Parallel design and choice of model
- OpenMP, parallel for over images, schedule(dynamic,1).
- Why OpenMP: shared-memory machine, images independent (embarrassingly parallel), minimal code change.
- Why not MPI: no cluster; would only add data distribution cost. Possible for multi-node scaling.
- Why not CUDA: needs GPU + cuFFT, large rewrite; possible future work.
- Race conditions: none - each iteration writes only to slot i (sums[i], outs[i]); no shared mutable state.
- cv::setNumThreads(1) to avoid nested parallelism / oversubscription.

## 5. Correctness
Checksum (weighted sum of all output pixels) is identical for sequential and OpenMP at every thread count (column "correct" in results.csv). Visual check with --save.

## 6. Experimental setup (write your own machine details)
CPU model, physical cores / logical threads, RAM, OS, compiler + flags (-O2 -fopenmp), OpenCV version, dataset (100 images, 512/1024/2048 px, blur radius 3-7 scaled with size).

## 7. Results (use results/summary.md and the PNG graphs)
- Execution time, speedup = T_seq / T_par, efficiency = speedup / threads
- Effect of threads (speedup.png, efficiency.png)
- Effect of number of images (time_vs_images.png)
- Effect of image size (time_vs_size.png)

## 8. Bottleneck analysis (explain what YOUR numbers show)
Things to check in your data:
- Where the time goes: stage_profile.png (Wiener deblur's FFTs usually dominate).
- Speedup flattening when threads > physical cores (hyper-threading helps less than real cores).
- Large images: FFT working set is big -> memory bandwidth limits scaling.
- Small image counts (e.g. 10 images with 8 threads): too few tasks, uneven finish -> load imbalance.
- Sequential part: image loading (load_s) is not parallelized -> Amdahl's law.

## 9. Conclusion and future work
Possible: parallelize loading, tile-level parallelism inside one image, MPI across nodes, CUDA with cuFFT.

## 10. Team contribution statement
One short paragraph per member (what they implemented / tested / measured).
