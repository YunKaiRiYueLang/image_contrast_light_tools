#include "lua_host.h"

extern "C"
{
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <fstream>

namespace
{
lua_State* g_lua_state = nullptr;
std::string g_lua_error;

bool ReadScriptFile(const std::wstring& path, std::string& out_source)
{
    std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
    if (!file)
        return false;

    const std::streamsize size = file.tellg();
    if (size < 0)
        return false;
    file.seekg(0, std::ios::beg);

    out_source.resize(static_cast<size_t>(size));
    if (size > 0 && !file.read(&out_source[0], size))
        return false;

    return true;
}

bool ReportLuaError()
{
    const char* message = lua_tostring(g_lua_state, -1);
    g_lua_error = (message != nullptr) ? message : "Unknown Lua error";
    lua_pop(g_lua_state, 1);
    return false;
}
}

bool InitializeLuaHost()
{
    if (g_lua_state != nullptr)
        return true;

    g_lua_state = luaL_newstate();
    if (g_lua_state == nullptr)
    {
        g_lua_error = "Failed to create Lua state";
        return false;
    }

    luaL_requiref(g_lua_state, LUA_GNAME, luaopen_base, 1);
    luaL_requiref(g_lua_state, LUA_TABLIBNAME, luaopen_table, 1);
    luaL_requiref(g_lua_state, LUA_STRLIBNAME, luaopen_string, 1);
    luaL_requiref(g_lua_state, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(g_lua_state, 4);
    return true;
}

void ShutdownLuaHost()
{
    if (g_lua_state != nullptr)
    {
        lua_close(g_lua_state);
        g_lua_state = nullptr;
    }
    g_lua_error.clear();
}

bool LoadAlgorithmInfoScript(const std::wstring& script_path)
{
    if (!InitializeLuaHost())
        return false;

    std::string source;
    if (!ReadScriptFile(script_path, source))
    {
        g_lua_error = "Failed to read Lua script";
        return false;
    }

    if (luaL_loadbufferx(g_lua_state, source.c_str(), source.size(), "algorithm_info", nullptr) != LUA_OK)
        return ReportLuaError();

    if (lua_pcall(g_lua_state, 0, 0, 0) != LUA_OK)
        return ReportLuaError();

    g_lua_error.clear();
    return true;
}

bool GetAlgorithmInfoFromLua(std::string& out_text)
{
    out_text.clear();

    if (g_lua_state == nullptr)
    {
        g_lua_error = "Lua host is not initialized";
        return false;
    }

    lua_getglobal(g_lua_state, "get_algo_info");
    if (!lua_isfunction(g_lua_state, -1))
    {
        lua_pop(g_lua_state, 1);
        g_lua_error = "get_algo_info is not defined";
        return false;
    }

    if (lua_pcall(g_lua_state, 0, 1, 0) != LUA_OK)
        return ReportLuaError();

    if (lua_type(g_lua_state, -1) != LUA_TSTRING)
    {
        lua_pop(g_lua_state, 1);
        g_lua_error = "get_algo_info must return a string";
        return false;
    }

    size_t length = 0;
    const char* text = lua_tolstring(g_lua_state, -1, &length);
    out_text.assign(text, length);
    lua_pop(g_lua_state, 1);

    g_lua_error.clear();
    return true;
}

std::string GetLuaErrorMessage()
{
    return g_lua_error;
}