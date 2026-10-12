// ============================================================
//  SEQUENTIAL version
//  Usage:
//    seq <folder> <numImages> [--save <outDir>] [--profile]
// ============================================================
#include "common.h"
#include "image_enhancement.h"

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: seq <folder> <numImages> "
                     "[--save <outDir>] [--profile]\n";
        return 1;
    }

    std::string folder = argv[1];
    size_t n = std::stoul(argv[2]);
    std::string saveDir;
    bool profile = false;
    for (int i = 3; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--save" && i + 1 < argc) saveDir = argv[++i];
        else if (a == "--profile") profile = true;
    }

    cv::setNumThreads(1);   // OpenCV must not use its own threads

    auto files = listImages(folder, n);
    if (files.empty()) { std::cerr << "No images in " << folder << "\n"; return 1; }

    double t0 = nowSeconds();
    std::vector<cv::Mat> images = loadImages(files);
    double loadSec = nowSeconds() - t0;
    const size_t count = images.size();

    // ---------- optional per-stage profile ----------
    if (profile)
    {
        double tEst = 0, tDeblur = 0, tSharp = 0;
        for (const auto& img : images)
        {
            double a = nowSeconds();
            int r = estimateBlurRadius(img);
            double b = nowSeconds();
            cv::Mat d = deblurImage(img, r);
            double c = nowSeconds();
            cv::Mat s = sharpenImage(d);
            double e = nowSeconds();
            tEst += b - a; tDeblur += c - b; tSharp += e - c;
        }
        std::cout << "PROFILE," << images[0].cols << "," << images[0].rows << ","
                  << 1000.0 * tEst / count << ","
                  << 1000.0 * tDeblur / count << ","
                  << 1000.0 * tSharp / count << std::endl;
        return 0;
    }

    // ---------- main timed loop ----------
    std::vector<double> sums(count, 0.0);
    std::vector<cv::Mat> outs;
    if (!saveDir.empty()) outs.resize(count);

    double start = nowSeconds();
    for (size_t i = 0; i < count; ++i)
    {
        cv::Mat result = enhanceImage(images[i]);   // auto radius + deblur + sharpen
        sums[i] = pixelSum(result);
        if (!saveDir.empty()) outs[i] = result;
    }
    double computeSec = nowSeconds() - start;

    // weighted total: also detects results landing in the wrong slot
    double checksum = 0;
    for (size_t i = 0; i < count; ++i) checksum += (i + 1) * sums[i];

    if (!saveDir.empty()) saveImages(files, outs, saveDir);

    printResult("seq", 1, count, images[0], computeSec, loadSec, checksum);
    return 0;
}
