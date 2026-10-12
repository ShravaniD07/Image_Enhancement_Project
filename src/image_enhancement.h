#ifndef IMAGE_ENHANCEMENT_H
#define IMAGE_ENHANCEMENT_H

#include <opencv2/opencv.hpp>

// Automatically estimate the defocus-blur radius (in pixels) of an image
int estimateBlurRadius(const cv::Mat& input);

// Operation 1: Wiener deconvolution (removes defocus blur)
cv::Mat deblurImage(const cv::Mat& input, int psfRadius, double noisePower = 0.02);

// Operation 2: High-boost sharpening (unsharp masking)
cv::Mat sharpenImage(const cv::Mat& input, double amount = 0.4, double sigma = 1.0);

// Full pipeline, fully automatic: estimate radius -> deblur -> sharpen
// If psfRadius <= 0 the radius is estimated from the image itself.
cv::Mat enhanceImage(const cv::Mat& input,
                     int psfRadius = 0,
                     double noisePower = 0.02,
                     double sharpenAmount = 0.4,
                     double sharpenSigma = 1.0);

#endif
