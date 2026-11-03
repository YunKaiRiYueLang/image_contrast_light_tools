#pragma once

#include <opencv2/core.hpp>
#include <string>

// Encodes and writes an image to disk using cv::imencode.
// The output format is inferred from the file extension (PNG when unknown).
bool SaveImageToFile(const cv::Mat& image, const std::wstring& path);