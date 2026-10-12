// Shared helpers used by BOTH the sequential and the OpenMP program.
#ifndef COMMON_H
#define COMMON_H

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

inline double nowSeconds()
{
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

inline bool isImageFile(const fs::path& p)
{
    std::string e = p.extension().string();
    std::transform(e.begin(), e.end(), e.begin(), ::tolower);
    return e == ".jpg" || e == ".jpeg" || e == ".png" || e == ".bmp";
}

// First `maxCount` images of a folder (sorted by name, so every run
// uses exactly the same images).
inline std::vector<fs::path> listImages(const fs::path& folder, size_t maxCount)
{
    std::vector<fs::path> files;
    if (fs::is_directory(folder))
        for (auto& e : fs::directory_iterator(folder))
            if (e.is_regular_file() && isImageFile(e.path()))
                files.push_back(e.path());
    std::sort(files.begin(), files.end());
    if (files.size() > maxCount) files.resize(maxCount);
    return files;
}

// Read all images into memory (so timing measures computation, not disk).
inline std::vector<cv::Mat> loadImages(const std::vector<fs::path>& files)
{
    std::vector<cv::Mat> images;
    images.reserve(files.size());
    for (const auto& f : files)
    {
        cv::Mat m = cv::imread(f.string(), cv::IMREAD_COLOR);
        if (m.empty())
        {
            std::cerr << "Cannot read: " << f << std::endl;
            std::exit(1);
        }
        images.push_back(m);
    }
    return images;
}

// Exact integer pixel sum of an 8-bit colour image.
// Used to prove that sequential and parallel give identical results.
inline double pixelSum(const cv::Mat& m)
{
    cv::Scalar s = cv::sum(m);
    return s[0] + s[1] + s[2];
}

// Save a list of result images (done AFTER timing, never timed).
inline void saveImages(const std::vector<fs::path>& files,
                       const std::vector<cv::Mat>& outs,
                       const fs::path& dir)
{
    fs::create_directories(dir);
    for (size_t i = 0; i < files.size(); ++i)
        cv::imwrite((dir / (files[i].stem().string() + "_enhanced.png")).string(),
                    outs[i]);
}

// One machine-readable line that the Python scripts parse.
inline void printResult(const std::string& mode, int threads, size_t images,
                        const cv::Mat& first, double computeSec,
                        double loadSec, double checksum)
{
    std::cout.setf(std::ios::fixed);
    std::cout.precision(4);
    std::cout << "RESULT," << mode << "," << threads << "," << images << ","
              << first.cols << "," << first.rows << ","
              << computeSec << "," << loadSec << ",";
    std::cout.precision(0);
    std::cout << checksum << std::endl;
}

#endif
