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

cv::Mat ContrastStretchPercentile(const cv::Mat& source, double low_pct, double high_pct)
{
    if (source.empty() || low_pct < 0.0 || high_pct > 100.0 || low_pct >= high_pct)
        return cv::Mat();

    cv::Mat image = source;
    if (image.depth() == CV_16U)
        image.convertTo(image, CV_8U, 1.0 / 256.0);
    else if (image.depth() == CV_32F)
        image.convertTo(image, CV_8U, 255.0);
    else if (image.depth() != CV_8U)
        image.convertTo(image, CV_8U);

    cv::Mat gray;
    if (image.channels() == 1)
        gray = image;
    else if (image.channels() == 3)
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    else if (image.channels() == 4)
        cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);
    else
        return cv::Mat();

    const int hist_size = 256;
    const float range[] = { 0.0f, 256.0f };
    const float* hist_range = range;
    cv::Mat hist;
    cv::calcHist(&gray, 1, 0, cv::Mat(), hist, 1, &hist_size, &hist_range);

    const double total = static_cast<double>(gray.total());
    const double low_count = total * low_pct / 100.0;
    const double high_count = total * high_pct / 100.0;

    int lo = 0;
    int hi = 255;
    double cumulative = 0.0;
    for (int i = 0; i < hist_size; ++i)
    {
        cumulative += hist.at<float>(i);
        if (cumulative >= low_count)
        {
            lo = i;
            break;
        }
    }

    cumulative = 0.0;
    for (int i = 0; i < hist_size; ++i)
    {
        cumulative += hist.at<float>(i);
        if (cumulative >= high_count)
        {
            hi = i;
            break;
        }
    }

    if (hi <= lo)
    {
        lo = 0;
        hi = 255;
    }

    cv::Mat lut(1, hist_size, CV_8U);
    const double scale = 255.0 / (hi - lo);
    for (int i = 0; i < hist_size; ++i)
        lut.at<uchar>(i) = cv::saturate_cast<uchar>((i - lo) * scale);

    cv::Mat result;
    if (image.channels() == 4)
    {
        std::vector<cv::Mat> channels;
        cv::split(image, channels);
        const cv::Mat alpha = channels[3];
        cv::Mat bgr;
        cv::merge(std::vector<cv::Mat>{ channels[0], channels[1], channels[2] }, bgr);
        cv::Mat stretched;
        cv::LUT(bgr, lut, stretched);
        std::vector<cv::Mat> stretched_channels;
        cv::split(stretched, stretched_channels);
        stretched_channels.push_back(alpha);
        cv::merge(stretched_channels, result);
    }
    else
    {
        cv::LUT(image, lut, result);
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