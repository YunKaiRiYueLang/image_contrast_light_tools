#pragma once

#include <string>

// Shows the native Windows Open dialog filtered to common image formats.
// Returns true when the user selects a file.
bool OpenImageFileDialog(std::wstring& out_path);

// Shows the native Windows Save dialog. PNG is the default format.
// Returns true when the user confirms a target path.
bool SaveImageFileDialog(std::wstring& out_path);

// Converts a wide-character path to UTF-8 for display purposes.
std::string WidePathToUtf8(const std::wstring& path);

// Returns the directory containing the running executable (with a trailing separator).
std::wstring GetApplicationDirectory();