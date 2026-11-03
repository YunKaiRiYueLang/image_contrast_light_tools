#include "file_dialog.h"

#include <windows.h>
#include <commdlg.h>

static std::string WideToUtf8(const wchar_t* text)
{
    if (text == nullptr || text[0] == L'\0')
        return std::string();

    const int length = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1)
        return std::string();

    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, &result[0], length, nullptr, nullptr);
    result.resize(static_cast<size_t>(length - 1));
    return result;
}

bool OpenImageFileDialog(std::wstring& out_path)
{
    wchar_t file_name[2048] = {};
    OPENFILENAMEW dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = GetActiveWindow();
    dialog.lpstrFilter = L"Image Files\0*.bmp;*.dib;*.jpg;*.jpeg;*.jpe;*.png;*.webp;*.tif;*.tiff;*.gif;*.jp2;*.pbm;*.pgm;*.ppm;*.pxm;*.pnm;*.pfm;*.sr;*.ras\0All Files\0*.*\0";
    dialog.lpstrFile = file_name;
    dialog.nMaxFile = static_cast<DWORD>(sizeof(file_name) / sizeof(file_name[0]));
    dialog.lpstrTitle = L"Open Image";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
    dialog.lpstrDefExt = L"png";

    if (!GetOpenFileNameW(&dialog))
        return false;

    out_path = file_name;
    return true;
}

std::string WidePathToUtf8(const std::wstring& path)
{
    return WideToUtf8(path.c_str());
}