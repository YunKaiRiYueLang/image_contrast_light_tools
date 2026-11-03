#include "image_io.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <fstream>
#include <vector>

static std::string ExtensionFromPath(const std::wstring& path)
{
    const std::wstring::size_type dot = path.find_last_of(L'.');
    const std::wstring::size_type slash = path.find_last_of(L"\\/");
    if (dot == std::wstring::npos || (slash != std::wstring::npos && dot < slash))
        return ".png";

    std::string extension;
    for (std::wstring::size_type i = dot; i < path.size(); ++i)
    {
        const wchar_t character = path[i];
        if (character > 0x7F)
            return ".png";
        extension.push_back(static_cast<char>(character));
    }

    for (char& character : extension)
    {
        if (character >= 'A' && character <= 'Z')
            character = static_cast<char>(character - 'A' + 'a');
    }
    return extension;
}

static cv::Mat ConvertTo8Bit(const cv::Mat& source)
{
    if (source.depth() == CV_8U)
        return source;

    cv::Mat result;
    if (source.depth() == CV_16U)
        source.convertTo(result, CV_8U, 1.0 / 256.0);
    else if (source.depth() == CV_32F)
        source.convertTo(result, CV_8U, 255.0);
    else
        source.convertTo(result, CV_8U);
    return result;
}

bool SaveImageToFile(const cv::Mat& image, const std::wstring& path)
{
    if (image.empty() || path.empty())
        return false;

    const std::string extension = ExtensionFromPath(path);
    cv::Mat export_image = ConvertTo8Bit(image);

    // JPEG does not support an alpha channel.
    if ((extension == ".jpg" || extension == ".jpeg") && export_image.channels() == 4)
    {
        cv::Mat bgr;
        cv::cvtColor(export_image, bgr, cv::COLOR_BGRA2BGR);
        export_image = bgr;
    }

    std::vector<unsigned char> encoded;
    try
    {
        if (!cv::imencode(extension, export_image, encoded))
            return false;
    }
    catch (const cv::Exception&)
    {
        return false;
    }

    std::ofstream file(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!file)
        return false;

    file.write(reinterpret_cast<const char*>(encoded.data()), static_cast<std::streamsize>(encoded.size()));
    return file.good();
}