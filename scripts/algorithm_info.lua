local kernel32 = dll.load("kernel32.dll")
local get_tick_count = nil
local get_tick_count64 = nil
if kernel32 then
    get_tick_count = dll.bind(kernel32, "GetTickCount", { ret = "uint32", args = {} })
    get_tick_count64 = dll.bind(kernel32, "GetTickCount64", { ret = "uint64", args = {} })
end

local msvcrt = dll.load("msvcrt.dll")
local abs_value = nil
if msvcrt then
    abs_value = dll.bind(msvcrt, "abs", { ret = "int32", args = { "int32" } })
end

local tick_count = nil
if get_tick_count then
    tick_count = get_tick_count()
end

local tick_count64 = nil
if get_tick_count64 then
    tick_count64 = get_tick_count64()
end

local abs_result = nil
if abs_value then
    abs_result = abs_value(-42)
end

local base_info = [[
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
  Percentile-clipped min-max stretch to [0, 255] (cv::calcHist + cv::LUT)]]

function get_algo_info()
    local lines = { base_info, "", "Windows DLL test" }

    if tick_count ~= nil then
        table.insert(lines, string.format("  kernel32!GetTickCount() = %u", tick_count))
    else
        table.insert(lines, "  kernel32!GetTickCount() = unavailable")
    end

    if tick_count64 ~= nil then
        table.insert(lines, string.format("  kernel32!GetTickCount64() = %u", tick_count64))
    else
        table.insert(lines, "  kernel32!GetTickCount64() = unavailable")
    end

    if abs_result ~= nil then
        table.insert(lines, string.format("  msvcrt!abs(-42) = %d", abs_result))
    else
        table.insert(lines, "  msvcrt!abs(-42) = unavailable")
    end

    return table.concat(lines, "\n")
end
function test_dll_functions()
    local lines = { "Windows DLL test" }

    if get_tick_count then
        table.insert(lines, string.format("  kernel32!GetTickCount() = %u", get_tick_count()))
    else
        table.insert(lines, "  kernel32!GetTickCount() = unavailable")
    end

    if get_tick_count64 then
        table.insert(lines, string.format("  kernel32!GetTickCount64() = %u", get_tick_count64()))
    else
        table.insert(lines, "  kernel32!GetTickCount64() = unavailable")
    end

    if abs_value then
        table.insert(lines, string.format("  msvcrt!abs(-42) = %d", abs_value(-42)))
    else
        table.insert(lines, "  msvcrt!abs(-42) = unavailable")
    end

    return table.concat(lines, "\n")
end