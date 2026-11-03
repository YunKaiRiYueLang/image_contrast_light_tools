#include "image_adjust.h"

#include <opencv2/imgproc.hpp>
#include <vector>

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
cv::Mat HistogramEqualization(const cv::Mat& source)
{
    if (source.empty())
        return cv::Mat();

    cv::Mat image = source;
    if (image.depth() == CV_16U)
        image.convertTo(image, CV_8U, 1.0 / 256.0);
    else if (image.depth() == CV_32F)
        image.convertTo(image, CV_8U, 255.0);
    else if (image.depth() != CV_8U)
        image.convertTo(image, CV_8U);

    if (image.channels() == 1)
    {
        cv::Mat result;
        cv::equalizeHist(image, result);
        return result;
    }

    if (image.channels() != 3 && image.channels() != 4)
        return cv::Mat();

    cv::Mat bgr;
    cv::Mat alpha;
    if (image.channels() == 4)
    {
        cv::cvtColor(image, bgr, cv::COLOR_BGRA2BGR);
        std::vector<cv::Mat> bgra;
        cv::split(image, bgra);
        alpha = bgra[3];
    }
    else
    {
        bgr = image;
    }

    cv::Mat ycrcb;
    cv::cvtColor(bgr, ycrcb, cv::COLOR_BGR2YCrCb);

    std::vector<cv::Mat> channels;
    cv::split(ycrcb, channels);
    cv::Mat equalizedY;
    cv::equalizeHist(channels[0], equalizedY);
    channels[0] = equalizedY;
    cv::merge(channels, ycrcb);

    cv::Mat result;
    cv::cvtColor(ycrcb, result, cv::COLOR_YCrCb2BGR);

    if (!alpha.empty())
    {
        std::vector<cv::Mat> bgrChannels;
        cv::split(result, bgrChannels);
        bgrChannels.push_back(alpha);
        cv::merge(bgrChannels, result);
    }

    return result;
}
cv::Mat ContrastLimitedAdaptiveHistogramEqualization(const cv::Mat& source, double clip_limit, int tile_grid_size)
{
    if (source.empty() || clip_limit <= 0.0 || tile_grid_size < 1)
        return cv::Mat();

    cv::Mat image = source;
    if (image.depth() == CV_16U)
        image.convertTo(image, CV_8U, 1.0 / 256.0);
    else if (image.depth() == CV_32F)
        image.convertTo(image, CV_8U, 255.0);
    else if (image.depth() != CV_8U)
        image.convertTo(image, CV_8U);

    const cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(clip_limit, cv::Size(tile_grid_size, tile_grid_size));

    if (image.channels() == 1)
    {
        cv::Mat result;
        clahe->apply(image, result);
        return result;
    }

    if (image.channels() != 3 && image.channels() != 4)
        return cv::Mat();

    cv::Mat bgr;
    cv::Mat alpha;
    if (image.channels() == 4)
    {
        cv::cvtColor(image, bgr, cv::COLOR_BGRA2BGR);
        std::vector<cv::Mat> bgra;
        cv::split(image, bgra);
        alpha = bgra[3];
    }
    else
    {
        bgr = image;
    }

    cv::Mat ycrcb;
    cv::cvtColor(bgr, ycrcb, cv::COLOR_BGR2YCrCb);

    std::vector<cv::Mat> channels;
    cv::split(ycrcb, channels);
    cv::Mat equalizedY;
    clahe->apply(channels[0], equalizedY);
    channels[0] = equalizedY;
    cv::merge(channels, ycrcb);

    cv::Mat result;
    cv::cvtColor(ycrcb, result, cv::COLOR_YCrCb2BGR);

    if (!alpha.empty())
    {
        std::vector<cv::Mat> bgrChannels;
        cv::split(result, bgrChannels);
        bgrChannels.push_back(alpha);
        cv::merge(bgrChannels, result);
    }

    return result;
}