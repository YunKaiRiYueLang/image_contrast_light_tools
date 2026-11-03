#include "image_adjust.h"

cv::Mat LinearBrightnessContrast(const cv::Mat& source, double alpha, double beta)
{
    if (source.empty())
        return cv::Mat();

    cv::Mat result;
    source.convertTo(result, -1, alpha, beta);
    return result;
}