#pragma once

#include <opencv2/core.hpp>

// Applies a linear transform: dst = src * alpha + beta, using cv::Mat::convertTo.
cv::Mat LinearBrightnessContrast(const cv::Mat& source, double alpha, double beta);