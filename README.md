# image_test

基于 Dear ImGui（GLFW + OpenGL3）与 OpenCV 的图像查看与调整工具。

## 功能

- `File -> Open`：调用 Windows 原生对话框打开图像。
- `Tool -> Save Current Image`：将当前显示效果编码写盘（默认 PNG）。
- `Tool -> Reset Image`：重置全部调整参数。
- `Tool -> algorithm_info`：调用 Lua 脚本 `scripts/algorithm_info.lua` 的 `get_algo_info()`，在只读文本框中显示算法说明，并通过 `dll.load` / `dll.bind` 调用 `kernel32.dll`、`msvcrt.dll` 的简单函数显示实测结果。
- `Tool -> test_dll_functions`：打开 `dll_test` 窗口，`run_test` 按钮调用 Lua 的 `test_dll_functions()`，每次点击重新执行 Windows DLL 调用并显示结果。
- `Tool -> Brightness and Contrast Adjustment`：
  - `Linear Brightness and Contrast Adjustment`
  - `Gamma Adjustment`
  - `Histogram Equalization`
  - `CLAHE`
  - `Contrast Stretching`
- 启动时自动加载可执行文件同目录下的 `lena.png`，缺失则跳过。

## 模块架构

界面层（`main.cpp`）只负责 UI 状态与 GL 纹理生命周期；算法、对话框、写盘各自独立成文件，三者之间零横向依赖。

```mermaid
graph TD
    subgraph L4["④ 平台与第三方库（外部依赖）"]
        GLFW["GLFW + OpenGL3<br/>窗口 / GL 上下文 / 纹理"]
        IMGUI["Dear ImGui<br/>imgui*.cpp + backends"]
        OCV["OpenCV 4.12<br/>core / imgproc / imgcodecs"]
        WIN["Win32 API<br/>commdlg.h 文件对话框<br/>kernel32 LoadLibraryW / GetProcAddress / FreeLibrary"]
        LUA["Lua 5.4.7<br/>脚本运行时（base / table / string / math）"]
    end

    subgraph L3["③ 应用编排层（仅此一个翻译单元）"]
        MAIN["main.cpp<br/>• 菜单 / 窗口 / 滑块状态<br/>• image_dirty 驱动重建<br/>• cv::Mat ↔ GL 纹理<br/>• 主循环与事件时序"]
    end

    subgraph L2["② 功能服务层（无 ImGui / 无 GL 依赖，可独立复用）"]
        FD["file_dialog.h / .cpp<br/>OpenImageFileDialog<br/>SaveImageFileDialog<br/>GetApplicationDirectory<br/>WidePathToUtf8"]
        IA["image_adjust.h / .cpp<br/>LinearBrightnessContrast<br/>GammaAdjustment<br/>HistogramEqualization<br/>CLAHE<br/>ContrastStretchPercentile"]
        IO["image_io.h / .cpp<br/>SaveImageToFile<br/>ExtensionFromPath<br/>ConvertTo8Bit"]
        LHOST["lua_host.h / .cpp<br/>InitializeLuaHost<br/>LoadAlgorithmInfoScript<br/>GetAlgorithmInfoFromLua<br/>ShutdownLuaHost"]
        LDLL["lua_dll.h / .cpp<br/>RegisterLuaDllApi<br/>dll.load / dll.bind / dll.close"]
    end

    MAIN -->|"#include file_dialog.h"| FD
    MAIN -->|"#include image_adjust.h"| IA
    MAIN -->|"#include image_io.h"| IO
    MAIN -->|"#include lua_host.h"| LHOST
    MAIN --> IMGUI
    MAIN --> GLFW
    MAIN --> OCV
    FD --> WIN
    IA --> OCV
    IO --> OCV
    LHOST --> LUA
    LHOST --> LDLL
    LDLL --> WIN

    FD -. "无依赖" .- IA
    IA -. "无依赖" .- IO
    FD -. "无依赖" .- IO
    LHOST -. "无依赖" .- FD
    LHOST -. "无依赖" .- IA
    LHOST -. "无依赖" .- IO
```

运行时脚本位于 `scripts/algorithm_info.lua`，由构建后事件复制到可执行文件目录的 `scripts/` 下。

## 编译单元依赖

头文件只暴露函数声明，`.cpp` 中的辅助函数（`WideToUtf8`、`ExtensionFromPath`、`ConvertTo8Bit`）均为 `static`，不进入其他翻译单元。

```mermaid
graph LR
    subgraph 源码
        main_cpp["main.cpp"]
        fd_cpp["file_dialog.cpp"]
        ia_cpp["image_adjust.cpp"]
        io_cpp["image_io.cpp"]
        lua_cpp["lua_host.cpp"]
        lua_dll_cpp["lua_dll.cpp"]
        fd_h["file_dialog.h"]
        ia_h["image_adjust.h"]
        io_h["image_io.h"]
        lua_h["lua_host.h"]
        lua_dll_h["lua_dll.h"]
    end
    main_cpp --> fd_h
    main_cpp --> ia_h
    main_cpp --> io_h
    main_cpp --> lua_h
    lua_cpp --> lua_h
    lua_cpp --> lua_dll_h
    lua_dll_cpp --> lua_dll_h
    lua_dll_cpp --> win_dll_h["windows.h"]
    lua_cpp --> lua_hdr["lua.h / lauxlib.h / lualib.h"]
    main_cpp --> imgui_h["imgui.h"]
    main_cpp --> impl_h["imgui_impl_glfw.h / imgui_impl_opengl3.h"]
    main_cpp --> glfw_h["GLFW/glfw3.h"]
    main_cpp --> ocv_h["opencv2/opencv.hpp"]
    fd_cpp --> fd_h
    fd_cpp --> win_h["windows.h / commdlg.h"]
    ia_cpp --> ia_h
    ia_cpp --> cv_imgproc["opencv2/imgproc.hpp"]
    io_cpp --> io_h
    io_cpp --> cv_imgcodecs["opencv2/imgcodecs.hpp"]
    main_cpp -.-x fd_cpp
    main_cpp -.-x ia_cpp
    main_cpp -.-x io_cpp
    main_cpp -.-x lua_cpp
```

## 运行时数据流

```mermaid
flowchart TD
    A["File → Open"] --> B["open_file_requested = true"]
    B --> C["OpenImageFileDialog()<br/>file_dialog"]
    C --> D["LoadImageFile()<br/>main.cpp 内 static：ifstream + cv::imdecode"]
    D --> E["source_image (原始 cv::Mat)"]

    E --> F{"image_dirty?"}
    G["滑块 / 勾选项变化"] --> F
    H["启动：GetApplicationDirectory() + lena.png"] --> D
    W["启动：GetApplicationDirectory() + scripts/algorithm_info.lua"] --> X["LoadAlgorithmInfoScript()<br/>lua_host"]

    F -->|是| I["LinearBrightnessContrast"]
    I --> J["GammaAdjustment"]
    J --> K["HistogramEqualization (可选)"]
    K --> L["CLAHE (可选)"]
    L --> M["ContrastStretchPercentile (可选)"]
    M --> N["CreateTextureFromImage<br/>ConvertToRgba8 → glTexImage2D"]
    N --> O["image_texture (GLuint)"]
    N --> P["displayed_image (当前显示效果)"]

    Q["Tool → Save Current Image"] --> R["SaveImageFileDialog()<br/>file_dialog"]
    R --> S["SaveImageToFile(displayed_image)<br/>image_io：ConvertTo8Bit → imencode → ofstream"]

    T["Tool → algorithm_info"] --> U["GetAlgorithmInfoFromLua()<br/>lua_host：pcall get_algo_info"]
    T2["Tool → test_dll_functions"] --> U5["CallLuaStringFunction('test_dll_functions')"]
    U5 --> U2
    U --> U2["脚本执行 dll.load / dll.bind"]
    U2 --> U3["Win32 LoadLibraryW + GetProcAddress"]
    U3 --> U4["调用 GetTickCount / abs 等"]
    U4 --> V["只读文本框显示"]
```

数据所有权：

| 变量 | 归属 | 含义 |
|---|---|---|
| `source_image` | `main.cpp` | 解码后的原始图，永不修改，是所有调整的输入 |
| `displayed_image` | `main.cpp` | 调整链末端结果，供保存使用（不是原始图） |
| `image_texture` | `main.cpp` → OpenGL | GPU 侧纹理，重建时先建新再删旧 |
| `image_dirty` | `main.cpp` | 唯一的“重算”触发信号，跨帧延迟重建 |

## 文件职责

| 文件 | 层 | 对外接口 | 内聚职责 | 依赖 |
|---|---|---|---|---|
| `main.cpp` | 编排/表现 | `main()` | 菜单与窗口、滑块状态、纹理生命周期、事件循环时序 | imgui、backends、GLFW、OpenCV |
| `file_dialog.h/.cpp` | 功能服务 | `OpenImageFileDialog`、`SaveImageFileDialog`、`GetApplicationDirectory`、`WidePathToUtf8` | Win32 原生对话框、宽字符路径转换、exe 目录定位 | `windows.h`、`commdlg.h` |
| `image_adjust.h/.cpp` | 功能服务 | 5 个 `cv::Mat → cv::Mat` 纯函数 | 全部像素运算（convertTo / pow / equalizeHist / createCLAHE / calcHist+LUT） | OpenCV `core`/`imgproc` |
| `image_io.h/.cpp` | 功能服务 | `SaveImageToFile` | 扩展名解析、位深归一、JPEG 去 alpha、imencode 写盘 | OpenCV `imgcodecs`/`imgproc`、`fstream` |
| `lua_host.h/.cpp` | 功能服务 | `InitializeLuaHost`、`LoadAlgorithmInfoScript`、`GetAlgorithmInfoFromLua`、`CallLuaStringFunction`、`ShutdownLuaHost`、`GetLuaErrorMessage` | Lua 状态生命周期、脚本读取与执行、通用无参字符串函数调用、错误收集；仅开放 base/table/string/math 库；初始化时注册 `dll` API | Lua 5.4.7、`fstream`、`lua_dll` |
| `lua_dll.h/.cpp` | 功能服务 | `RegisterLuaDllApi`；Lua 侧 `dll.load` / `dll.bind` / `dll.close` | 通用 DLL 加载与按签名分派调用；不包含任何具体 DLL 名、路径或符号 | `windows.h`、Lua 5.4.7 |
| `scripts/algorithm_info.lua` | 脚本资源 | `get_algo_info()`、`test_dll_functions()` | 描述 DLL 路径、符号与签名，返回算法说明文本和 Windows DLL 实测结果；修改后无需重新编译 exe | — |

## Lua DLL API

C++ 侧只注册通用原语，具体 DLL 由脚本决定：

```lua
local kernel32 = dll.load("kernel32.dll")
local get_tick_count = dll.bind(kernel32, "GetTickCount", { ret = "uint32", args = {} })
local ticks = get_tick_count()
```

- `dll.load(path)`：成功返回句柄；失败返回 `nil, error`。
- `dll.bind(handle, symbol, signature)`：`ret` 支持 `void`、`int32`、`uint32`、`int64`、`uint64`、`double`、`ptr`、`bool`；`args` 为类型数组，最多 4 个。
- `dll.close(handle)`：主动卸载；未调用时由 `__gc` 释放。
- 导出函数需使用 C 链接（`extern "C"`）；x64 下整数参数按 64 位传递。

## 构建

项目提供 Visual Studio 工程 `example_glfw_opengl3.vcxproj`（MSVC v143，x64）以及 `Makefile` / `build_win32.bat`。Lua 5.4.7 源码直接位于 `third_party/lua-5.4.7/src` 并已加入工程，无需预编译静态库；构建后事件会把 `scripts/` 复制到输出目录。

首次构建前需将工程属性中的 `OpenCV_DIR` 指向本机 OpenCV 4.12 的构建目录（当前为 `D:\opencv\4.12\opencv\build`），并确保 `opencv_world4120.dll` 与可执行文件同目录。

## 依赖

- Dear ImGui（仓库根目录）
- GLFW（`examples/libs/glfw`）
- OpenCV 4.12（`opencv_world4120`，x64/vc16）
- Lua 5.4.7（`third_party/lua-5.4.7/src`，源码内置，MIT）
- `comdlg32.lib`（原生打开 / 保存对话框）