#pragma once

#include <opencv2/core.hpp>

// Applies a linear transform: dst = src * alpha + beta, using cv::Mat::convertTo.
cv::Mat LinearBrightnessContrast(const cv::Mat& source, double alpha, double beta);

// Applies gamma correction: dst = pow(src / max_value, gamma) * max_value, using cv::pow.
cv::Mat GammaAdjustment(const cv::Mat& source, double gamma);
// Applies histogram equalization using cv::equalizeHist (on the Y channel for color images).
cv::Mat HistogramEqualization(const cv::Mat& source);