#include "image_adjust.h"

cv::Mat LinearBrightnessContrast(const cv::Mat& source, double alpha, double beta)
{
    if (source.empty())
        return cv::Mat();

    cv::Mat result;
    source.convertTo(result, -1, alpha, beta);
    return result;
}

cv::Mat GammaAdjustment(const cv::Mat& source, double gamma)
{
    if (source.empty() || gamma <= 0.0)
        return cv::Mat();

    double scale = 1.0;
    cv::Mat normalized;
    if (source.depth() == CV_8U)
    {
        scale = 255.0;
        source.convertTo(normalized, CV_32F, 1.0 / scale);
    }
    else if (source.depth() == CV_16U)
    {
        scale = 65535.0;
        source.convertTo(normalized, CV_32F, 1.0 / scale);
    }
    else if (source.depth() == CV_32F)
    {
        source.convertTo(normalized, CV_32F);
    }
    else
    {
        return cv::Mat();
    }

    cv::pow(normalized, gamma, normalized);

    cv::Mat result;
    normalized.convertTo(result, source.type(), scale);
    return result;
}