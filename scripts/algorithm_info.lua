function get_algo_info()
    return [[
Available algorithms

Linear Brightness and Contrast Adjustment
  dst = src * Contrast + Brightness (cv::Mat::convertTo)

Gamma Adjustment
  dst = pow(src / max_value, Gamma) * max_value (cv::pow)

Histogram Equalization
  cv::equalizeHist on the Y channel for color images

CLAHE
  cv::createCLAHE(Clip Limit, Tile Grid Size) on the Y channel

Contrast Stretching
  Percentile-clipped min-max stretch to [0, 255] (cv::calcHist + cv::LUT)
]]
end