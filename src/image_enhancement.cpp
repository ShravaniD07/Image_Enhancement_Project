#include "image_enhancement.h"

#include <algorithm>
#include <cmath>
#include <vector>

// ---------------------------------------------------------
// Disk-shaped PSF (models defocus blur)
// ---------------------------------------------------------
static cv::Mat createDiskPSF(int radius)
{
    int size = 2 * radius + 1;
    cv::Mat psf = cv::Mat::zeros(size, size, CV_32F);

    int count = 0;
    for (int y = -radius; y <= radius; ++y)
        for (int x = -radius; x <= radius; ++x)
            if (x * x + y * y <= radius * radius)
            {
                psf.at<float>(y + radius, x + radius) = 1.0f;
                count++;
            }

    psf /= static_cast<float>(count);
    return psf;
}

// Place PSF in a full-size image with its center at (0,0)
static cv::Mat shiftPSF(const cv::Mat& psf, cv::Size imageSize)
{
    cv::Mat full = cv::Mat::zeros(imageSize, CV_32F);
    int cx = psf.cols / 2, cy = psf.rows / 2;

    for (int y = 0; y < psf.rows; ++y)
        for (int x = 0; x < psf.cols; ++x)
        {
            int sx = ((x - cx) % imageSize.width + imageSize.width) % imageSize.width;
            int sy = ((y - cy) % imageSize.height + imageSize.height) % imageSize.height;
            full.at<float>(sy, sx) = psf.at<float>(y, x);
        }
    return full;
}

// ---------------------------------------------------------
// Automatic blur-radius estimation
//
// A disk blur makes the image's frequency spectrum have a dip
// (a "zero") at frequency f = 0.61 / radius.
// We look at the radially averaged log-spectrum and find which
// radius gives the deepest dip at its expected position.
// ---------------------------------------------------------
int estimateBlurRadius(const cv::Mat& input)
{
    cv::Mat gray;
    cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    gray.convertTo(gray, CV_32F, 1.0 / 255.0);
    gray -= cv::mean(gray)[0];

    // Hann window to avoid edge artifacts in the spectrum
    cv::Mat win;
    cv::createHanningWindow(win, gray.size(), CV_32F);
    gray = gray.mul(win);

    cv::Mat spec;
    cv::dft(gray, spec, cv::DFT_COMPLEX_OUTPUT);

    const int h = gray.rows, w = gray.cols;
    const int nb = std::clamp(std::max(h, w) / 2, 100, 1000);

    std::vector<double> sum(nb, 0.0);
    std::vector<int> cnt(nb, 0);

    for (int y = 0; y < h; ++y)
    {
        double fy = (y <= h / 2 ? y : y - h) / static_cast<double>(h);
        const cv::Vec2f* row = spec.ptr<cv::Vec2f>(y);
        for (int x = 0; x < w; ++x)
        {
            double fx = (x <= w / 2 ? x : x - w) / static_cast<double>(w);
            double f = std::sqrt(fx * fx + fy * fy);
            if (f >= 0.5) continue;

            int b = static_cast<int>(f / 0.5 * nb);
            double p = row[x][0] * row[x][0] + row[x][1] * row[x][1];
            sum[b] += std::log(p + 1e-12);
            cnt[b]++;
        }
    }

    cv::Mat prof(nb, 1, CV_32F);
    for (int i = 0; i < nb; ++i)
        prof.at<float>(i) = cnt[i] ? static_cast<float>(sum[i] / cnt[i])
                                   : (i ? prof.at<float>(i - 1) : 0.f);

    cv::GaussianBlur(prof, prof, cv::Size(1, 0), 1.5, 1.5);

    const int rMin = 2;
    const int rMax = std::clamp(std::max(h, w) / 50, 12, 40);

    double bestDip = -1e30;
    int bestR = 5;   // fallback

    for (int r = rMin; r <= rMax; ++r)
    {
        double f0 = 0.61 / r;
        int i0 = static_cast<int>(f0 / 0.5 * nb);
        int wd = std::max(2, static_cast<int>(0.15 * i0));
        if (i0 - wd < 0 || i0 + wd >= nb) continue;

        double dip = 0.5 * (prof.at<float>(i0 - wd) + prof.at<float>(i0 + wd))
                     - prof.at<float>(i0);
        if (dip > bestDip)
        {
            bestDip = dip;
            bestR = r;
        }
    }
    return bestR;
}

// ---------------------------------------------------------
// Wiener deconvolution on ONE float channel (values 0..1)
// F = G * conj(H) / (|H|^2 + K)
// ---------------------------------------------------------
static cv::Mat wienerDeconvolution(const cv::Mat& input,
                                   int psfRadius,
                                   double noisePower)
{
    CV_Assert(input.type() == CV_32F);

    // Mirror-pad to avoid ringing at the borders
    int pad = psfRadius * 2;
    cv::Mat padded;
    cv::copyMakeBorder(input, padded, pad, pad, pad, pad, cv::BORDER_REFLECT);

    cv::Mat psf = shiftPSF(createDiskPSF(psfRadius), padded.size());

    cv::Mat G, H;
    cv::dft(padded, G, cv::DFT_COMPLEX_OUTPUT);
    cv::dft(psf, H, cv::DFT_COMPLEX_OUTPUT);

    cv::Mat num;
    cv::mulSpectrums(G, H, num, 0, true);   // G * conj(H)

    std::vector<cv::Mat> h, n;
    cv::split(H, h);
    cv::split(num, n);

    cv::Mat denom = h[0].mul(h[0]) + h[1].mul(h[1]) +
                    static_cast<float>(noisePower);

    cv::divide(n[0], denom, n[0]);
    cv::divide(n[1], denom, n[1]);

    cv::Mat resultDFT;
    cv::merge(n, resultDFT);

    cv::Mat result;
    cv::idft(resultDFT, result, cv::DFT_REAL_OUTPUT | cv::DFT_SCALE);

    result = result(cv::Rect(pad, pad, input.cols, input.rows)).clone();

    result.setTo(0.0f, result < 0.0f);
    result.setTo(1.0f, result > 1.0f);
    return result;
}

// ---------------------------------------------------------
// Operation 1: Deblur (brightness channel only)
// ---------------------------------------------------------
cv::Mat deblurImage(const cv::Mat& input, int psfRadius, double noisePower)
{
    if (input.empty()) return cv::Mat();

    if (psfRadius <= 0)
        psfRadius = estimateBlurRadius(input);

    cv::Mat ycrcb;
    cv::cvtColor(input, ycrcb, cv::COLOR_BGR2YCrCb);

    std::vector<cv::Mat> ch;
    cv::split(ycrcb, ch);

    cv::Mat yFloat;
    ch[0].convertTo(yFloat, CV_32F, 1.0 / 255.0);

    cv::Mat restored = wienerDeconvolution(yFloat, psfRadius, noisePower);
    restored.convertTo(ch[0], CV_8U, 255.0);

    cv::merge(ch, ycrcb);

    cv::Mat out;
    cv::cvtColor(ycrcb, out, cv::COLOR_YCrCb2BGR);
    return out;
}

// ---------------------------------------------------------
// Operation 2: High-boost sharpening (brightness channel only)
// output = original + amount * (original - blurred)
// ---------------------------------------------------------
cv::Mat sharpenImage(const cv::Mat& input, double amount, double sigma)
{
    if (input.empty()) return cv::Mat();

    cv::Mat ycrcb;
    cv::cvtColor(input, ycrcb, cv::COLOR_BGR2YCrCb);

    std::vector<cv::Mat> ch;
    cv::split(ycrcb, ch);

    cv::Mat blurred, sharp;
    cv::GaussianBlur(ch[0], blurred, cv::Size(0, 0), sigma);
    cv::addWeighted(ch[0], 1.0 + amount, blurred, -amount, 0, sharp);

    ch[0] = sharp;
    cv::merge(ch, ycrcb);

    cv::Mat out;
    cv::cvtColor(ycrcb, out, cv::COLOR_YCrCb2BGR);
    return out;
}

// ---------------------------------------------------------
// Full pipeline
// ---------------------------------------------------------
cv::Mat enhanceImage(const cv::Mat& input,
                     int psfRadius,
                     double noisePower,
                     double sharpenAmount,
                     double sharpenSigma)
{
    cv::Mat deblurred = deblurImage(input, psfRadius, noisePower);
    return sharpenImage(deblurred, sharpenAmount, sharpenSigma);
}
