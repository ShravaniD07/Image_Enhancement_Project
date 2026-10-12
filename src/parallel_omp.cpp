// ============================================================
//  PARALLEL version (OpenMP)
//
//  Strategy: DATA PARALLELISM ACROSS IMAGES.
//  Every image is independent, so each thread takes a different
//  image and runs the whole pipeline (estimate radius -> Wiener
//  deblur -> sharpen) on it. No communication, no locks.
//
//  Usage:
//    omp <folder> <numImages> <threads> [--save <outDir>]
// ============================================================
#include <omp.h>

#include "common.h"
#include "image_enhancement.h"

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        std::cerr << "Usage: omp <folder> <numImages> <threads> "
                     "[--save <outDir>]\n";
        return 1;
    }

    std::string folder = argv[1];
    size_t n = std::stoul(argv[2]);
    int threads = std::stoi(argv[3]);
    std::string saveDir;
    for (int i = 4; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--save" && i + 1 < argc) saveDir = argv[++i];
    }

    // OpenCV must stay single-threaded: OpenMP controls the parallelism.
    cv::setNumThreads(1);
    omp_set_num_threads(threads);

    auto files = listImages(folder, n);
    if (files.empty()) { std::cerr << "No images in " << folder << "\n"; return 1; }

    double t0 = nowSeconds();
    std::vector<cv::Mat> images = loadImages(files);   // sequential disk read
    double loadSec = nowSeconds() - t0;
    const int count = static_cast<int>(images.size());

    std::vector<double> sums(count, 0.0);
    std::vector<cv::Mat> outs;
    if (!saveDir.empty()) outs.resize(count);

    double start = nowSeconds();

    // Each iteration writes only to its own slot i -> no race condition.
    // schedule(dynamic,1): a thread that finishes early grabs the next
    // image, which balances the load if images have different cost.
    #pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < count; ++i)
    {
        cv::Mat result = enhanceImage(images[i]);
        sums[i] = pixelSum(result);
        if (!saveDir.empty()) outs[i] = result;
    }

    double computeSec = nowSeconds() - start;

    double checksum = 0;
    for (int i = 0; i < count; ++i) checksum += (i + 1) * sums[i];

    if (!saveDir.empty()) saveImages(files, outs, saveDir);

    printResult("omp", threads, count, images[0], computeSec, loadSec, checksum);
    return 0;
}
