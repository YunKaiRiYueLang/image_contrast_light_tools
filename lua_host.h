#pragma once

#include <string>

bool InitializeLuaHost();
void ShutdownLuaHost();
bool LoadAlgorithmInfoScript(const std::wstring& script_path);
bool GetAlgorithmInfoFromLua(std::string& out_text);
std::string GetLuaErrorMessage();