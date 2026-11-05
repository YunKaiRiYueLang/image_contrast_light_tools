// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "file_dialog.h"
#include "image_adjust.h"
#include "image_io.h"
#include "lua_host.h"
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <fstream>
#include <stdio.h>
#include <string>
#include <vector>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static cv::Mat ConvertToRgba8(const cv::Mat& source)
{
    cv::Mat image = source;

    if (image.depth() == CV_16U)
        image.convertTo(image, CV_8U, 1.0 / 256.0);
    else if (image.depth() == CV_32F)
        image.convertTo(image, CV_8U, 255.0);
    else if (image.depth() != CV_8U)
        image.convertTo(image, CV_8U);

    cv::Mat rgba;
    if (image.channels() == 1)
        cv::cvtColor(image, rgba, cv::COLOR_GRAY2RGBA);
    else if (image.channels() == 3)
        cv::cvtColor(image, rgba, cv::COLOR_BGR2RGBA);
    else if (image.channels() == 4)
        cv::cvtColor(image, rgba, cv::COLOR_BGRA2RGBA);
    else
        return cv::Mat();

    return rgba;
}

static bool LoadImageFile(const std::wstring& path, cv::Mat& out_image)
{
    std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
    if (!file)
        return false;

    const std::streamsize file_size = file.tellg();
    if (file_size <= 0)
        return false;
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> encoded(static_cast<size_t>(file_size));
    if (!file.read(reinterpret_cast<char*>(encoded.data()), file_size))
        return false;

    cv::Mat decoded = cv::imdecode(encoded, cv::IMREAD_UNCHANGED);
    if (decoded.empty())
        return false;

    out_image = decoded;
    return true;
}

static bool CreateTextureFromImage(const cv::Mat& image, GLuint* out_texture, int* out_width, int* out_height)
{
    const cv::Mat rgba = ConvertToRgba8(image);
    if (rgba.empty())
        return false;

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba.cols, rgba.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data);
    glBindTexture(GL_TEXTURE_2D, 0);

    *out_texture = texture;
    *out_width = rgba.cols;
    *out_height = rgba.rows;
    return true;
}

// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Select GL version + let the backend select a GLSL version
    const char* glsl_version = nullptr;
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + generally GLSL 150
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + generally GLSL 130
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    GLFWwindow* window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "image_test", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_linear_adjust = false;
    bool show_gamma_adjust = false;
    bool show_histogram_equalization = false;
    bool show_clahe = false;
    bool show_contrast_stretching = false;
    bool show_algorithm_info = false;
    bool open_file_requested = false;
    bool save_file_requested = false;
    bool image_dirty = false;
    float linear_alpha = 1.0f;
    float linear_beta = 0.0f;
    float gamma = 1.0f;
    float clahe_clip_limit = 2.0f;
    int clahe_tile_grid_size = 8;
    float stretch_low_percent = 1.0f;
    float stretch_high_percent = 99.0f;

    cv::Mat source_image;
    cv::Mat displayed_image;
    GLuint image_texture = 0;
    int image_width = 0;
    int image_height = 0;
    std::string image_error;
    const ImVec4 clear_color = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

    const std::wstring algorithm_info_script_path = GetApplicationDirectory() + L"scripts\\algorithm_info.lua";
    LoadAlgorithmInfoScript(algorithm_info_script_path);

    // Load the default image from the executable directory when available
    {
        const std::wstring default_image_path = GetApplicationDirectory() + L"lena.png";
        if (LoadImageFile(default_image_path, source_image))
            image_dirty = true;
    }

    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Rebuild the texture when the adjustment parameters change
        if (image_dirty)
        {
            image_dirty = false;
            if (!source_image.empty())
            {
                const cv::Mat linear = LinearBrightnessContrast(source_image, linear_alpha, linear_beta);
                cv::Mat adjusted = (gamma == 1.0f) ? linear : GammaAdjustment(linear, gamma);
                if (show_histogram_equalization)
                {
                    const cv::Mat equalized = HistogramEqualization(adjusted);
                    if (!equalized.empty())
                        adjusted = equalized;
                }
                if (show_clahe)
                {
                    const cv::Mat clahe_adjusted = ContrastLimitedAdaptiveHistogramEqualization(adjusted, clahe_clip_limit, clahe_tile_grid_size);
                    if (!clahe_adjusted.empty())
                        adjusted = clahe_adjusted;
                }
                if (show_contrast_stretching)
                {
                    const cv::Mat stretched = ContrastStretchPercentile(adjusted, stretch_low_percent, stretch_high_percent);
                    if (!stretched.empty())
                        adjusted = stretched;
                }
                GLuint texture = 0;
                int width = 0;
                int height = 0;
                if (CreateTextureFromImage(adjusted, &texture, &width, &height))
                {
                    if (image_texture != 0)
                        glDeleteTextures(1, &image_texture);
                    image_texture = texture;
                    image_width = width;
                    image_height = height;
                    displayed_image = adjusted;
                    image_error.clear();
                }
                else
                {
                    image_error = "Failed to create texture";
                }
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Main application window, filling the whole viewport
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        ImGuiWindowFlags main_window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

        ImGui::Begin("Main##MainWindow", nullptr, main_window_flags);

        // Menu bar
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open"))
                    open_file_requested = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Tool"))
            {
                if (ImGui::MenuItem("Save Current Image", nullptr, false, !displayed_image.empty()))
                    save_file_requested = true;
                if (ImGui::MenuItem("Reset Image"))
                {
                    linear_alpha = 1.0f;
                    linear_beta = 0.0f;
                    gamma = 1.0f;
                    show_histogram_equalization = false;
                    show_clahe = false;
                    clahe_clip_limit = 2.0f;
                    clahe_tile_grid_size = 8;
                    show_contrast_stretching = false;
                    stretch_low_percent = 1.0f;
                    stretch_high_percent = 99.0f;

                    image_dirty = true;
                }
                if (ImGui::MenuItem("algorithm_info"))
                {
                    LoadAlgorithmInfoScript(algorithm_info_script_path);
                    show_algorithm_info = true;
                }
                ImGui::Separator();
                if (ImGui::BeginMenu("Brightness and Contrast Adjustment"))
                {
                    ImGui::MenuItem("Linear Brightness and Contrast Adjustment", nullptr, &show_linear_adjust);
                    ImGui::MenuItem("Gamma Adjustment", nullptr, &show_gamma_adjust);
                    if (ImGui::MenuItem("Histogram Equalization", nullptr, &show_histogram_equalization))
                        image_dirty = true;
                    if (ImGui::MenuItem("CLAHE", nullptr, &show_clahe))
                        image_dirty = true;
                    if (ImGui::MenuItem("Contrast Stretching", nullptr, &show_contrast_stretching))
                        image_dirty = true;
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Image preview area
        ImVec2 preview_size = ImGui::GetContentRegionAvail();
        ImGui::BeginChild("PreviewArea", preview_size, ImGuiChildFlags_Borders);
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 cursor = ImGui::GetCursorPos();
            if (image_texture != 0 && image_width > 0 && image_height > 0)
            {
                const float scale = std::min(avail.x / (float)image_width, avail.y / (float)image_height);
                const ImVec2 image_size((float)image_width * scale, (float)image_height * scale);
                ImGui::SetCursorPos(ImVec2(cursor.x + (avail.x - image_size.x) * 0.5f, cursor.y + (avail.y - image_size.y) * 0.5f));
                ImGui::Image(static_cast<ImTextureID>(image_texture), image_size);
            }
            else
            {
                const char* message = image_error.empty() ? "No image loaded" : image_error.c_str();
                const ImVec2 text_size = ImGui::CalcTextSize(message);
                ImGui::SetCursorPos(ImVec2(cursor.x + (avail.x - text_size.x) * 0.5f, cursor.y + (avail.y - text_size.y) * 0.5f));
                if (image_error.empty())
                    ImGui::TextDisabled("%s", message);
                else
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", message);
            }
        }
        ImGui::EndChild();

        ImGui::End();

        // Linear brightness and contrast adjustment window
        if (show_linear_adjust)
        {
            if (ImGui::Begin("Linear Brightness and Contrast Adjustment", &show_linear_adjust, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if (ImGui::SliderFloat("Contrast", &linear_alpha, 0.0f, 3.0f, "%.3f"))
                    image_dirty = true;
                if (ImGui::SliderFloat("Brightness", &linear_beta, -255.0f, 255.0f, "%.1f"))
                    image_dirty = true;
                //ImGui::TextDisabled("dst = src * alpha + beta");
            }
            ImGui::End();
        }

        // Gamma adjustment window
        if (show_gamma_adjust)
        {
            if (ImGui::Begin("Gamma Adjustment", &show_gamma_adjust, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if (ImGui::SliderFloat("Gamma", &gamma, 0.1f, 5.0f, "%.2f"))
                    image_dirty = true;
            }
            ImGui::End();
        }

        // Histogram equalization window
        if (show_histogram_equalization)
        {
            if (ImGui::Begin("Histogram Equalization", &show_histogram_equalization, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::TextDisabled("Applies histogram equalization to the current image.");
            }
            ImGui::End();
            if (!show_histogram_equalization)
                image_dirty = true;
        }

        // CLAHE window
        if (show_clahe)
        {
            if (ImGui::Begin("CLAHE", &show_clahe, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if (ImGui::SliderFloat("Clip Limit", &clahe_clip_limit, 0.1f, 10.0f, "%.2f"))
                    image_dirty = true;
                if (ImGui::SliderInt("Tile Grid Size", &clahe_tile_grid_size, 2, 32))
                    image_dirty = true;
            }
            ImGui::End();
            if (!show_clahe)
                image_dirty = true;
        }

        // Contrast stretching window
        if (show_contrast_stretching)
        {
            if (ImGui::Begin("Contrast Stretching", &show_contrast_stretching, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if (ImGui::SliderFloat("Low Percentile", &stretch_low_percent, 0.0f, 10.0f, "%.1f%%"))
                    image_dirty = true;
                if (ImGui::SliderFloat("High Percentile", &stretch_high_percent, 90.0f, 100.0f, "%.1f%%"))
                    image_dirty = true;
                ImGui::TextDisabled("Percentile-clipped linear histogram stretching");
            }
            ImGui::End();
            if (!show_contrast_stretching)
                image_dirty = true;
        }

        // Algorithm info window
        if (show_algorithm_info)
        {
            ImGui::SetNextWindowSize(ImVec2(600.0f, 320.0f), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("algorithm_info", &show_algorithm_info))
            {
                std::string algorithm_info_text;
                if (!GetAlgorithmInfoFromLua(algorithm_info_text) || algorithm_info_text.empty())
                    algorithm_info_text = GetLuaErrorMessage();
                if (algorithm_info_text.empty())
                    algorithm_info_text = "Failed to get algorithm information";
                ImGui::InputTextMultiline("##algorithm_info_text", &algorithm_info_text[0], algorithm_info_text.size() + 1, ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_ReadOnly);
            }
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        // Save the currently displayed image (kept outside of the ImGui frame)
        if (save_file_requested)
        {
            save_file_requested = false;
            std::wstring target_path;
            if (SaveImageFileDialog(target_path))
            {
                if (!SaveImageToFile(displayed_image, target_path))
                    image_error = "Failed to save image";
            }
        }

        // Native file dialog (kept outside of the ImGui frame)
        if (open_file_requested)
        {
            open_file_requested = false;
            std::wstring selected_path;
            if (OpenImageFileDialog(selected_path))
            {
                if (LoadImageFile(selected_path, source_image))
                {
                    linear_alpha = 1.0f;
                    linear_beta = 0.0f;
                    gamma = 1.0f;
                    image_dirty = true;
                }
                else
                {
                    image_error = "Failed to load image";
                }
            }
        }
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    if (image_texture != 0)
        glDeleteTextures(1, &image_texture);

    ShutdownLuaHost();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
