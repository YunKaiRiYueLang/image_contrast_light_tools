#include "lua_dll.h"

extern "C"
{
#include "lauxlib.h"
#include "lua.h"
}

#include <windows.h>

#include <cstdint>
#include <new>
#include <string>
#include <vector>

namespace
{
const char* const kHandleMetatable = "image_test.dll_handle";
const char* const kFunctionMetatable = "image_test.dll_function";
const int kMaxArguments = 4;

enum class ValueKind
{
    Void,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Double,
    Pointer,
    Boolean,
};

enum class SlotKind
{
    Integer,
    Double,
};

struct Slot
{
    SlotKind kind = SlotKind::Integer;
    long long integer_value = 0;
    double double_value = 0.0;
};

struct DllHandle
{
    HMODULE module = nullptr;
};

struct DllFunction
{
    FARPROC proc = nullptr;
    ValueKind return_kind = ValueKind::Void;
    std::vector<ValueKind> arg_kinds;
};

std::wstring Utf8ToWide(const char* text)
{
    if (text == nullptr || text[0] == '\0')
        return std::wstring();

    const int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (length <= 0)
        return std::wstring();

    std::vector<wchar_t> buffer(static_cast<size_t>(length));
    MultiByteToWideChar(CP_UTF8, 0, text, -1, buffer.data(), length);
    return std::wstring(buffer.data(), buffer.size() - 1);
}

bool ParseValueKind(const char* text, ValueKind& out_kind)
{
    const std::string name(text);
    if (name == "void")
        out_kind = ValueKind::Void;
    else if (name == "int32")
        out_kind = ValueKind::Int32;
    else if (name == "uint32")
        out_kind = ValueKind::UInt32;
    else if (name == "int64")
        out_kind = ValueKind::Int64;
    else if (name == "uint64")
        out_kind = ValueKind::UInt64;
    else if (name == "double")
        out_kind = ValueKind::Double;
    else if (name == "ptr")
        out_kind = ValueKind::Pointer;
    else if (name == "bool")
        out_kind = ValueKind::Boolean;
    else
        return false;
    return true;
}

template <typename Ret, typename... Args>
Ret CallProc(FARPROC proc, Args... args)
{
    using Function = Ret (*)(Args...);
    return reinterpret_cast<Function>(proc)(args...);
}

template <typename Ret>
Ret Invoke2(FARPROC proc, const Slot* slots)
{
    const bool first_double = slots[0].kind == SlotKind::Double;
    const bool second_double = slots[1].kind == SlotKind::Double;

    if (first_double)
    {
        if (second_double)
            return CallProc<Ret, double, double>(proc, slots[0].double_value, slots[1].double_value);
        return CallProc<Ret, double, long long>(proc, slots[0].double_value, slots[1].integer_value);
    }
    if (second_double)
        return CallProc<Ret, long long, double>(proc, slots[0].integer_value, slots[1].double_value);
    return CallProc<Ret, long long, long long>(proc, slots[0].integer_value, slots[1].integer_value);
}

template <typename Ret>
Ret Invoke3(FARPROC proc, const Slot* slots)
{
    const int mask = (slots[0].kind == SlotKind::Double ? 1 : 0)
        | (slots[1].kind == SlotKind::Double ? 2 : 0)
        | (slots[2].kind == SlotKind::Double ? 4 : 0);

    switch (mask)
    {
    case 0:
        return CallProc<Ret, long long, long long, long long>(proc, slots[0].integer_value, slots[1].integer_value, slots[2].integer_value);
    case 1:
        return CallProc<Ret, double, long long, long long>(proc, slots[0].double_value, slots[1].integer_value, slots[2].integer_value);
    case 2:
        return CallProc<Ret, long long, double, long long>(proc, slots[0].integer_value, slots[1].double_value, slots[2].integer_value);
    case 3:
        return CallProc<Ret, double, double, long long>(proc, slots[0].double_value, slots[1].double_value, slots[2].integer_value);
    case 4:
        return CallProc<Ret, long long, long long, double>(proc, slots[0].integer_value, slots[1].integer_value, slots[2].double_value);
    case 5:
        return CallProc<Ret, double, long long, double>(proc, slots[0].double_value, slots[1].integer_value, slots[2].double_value);
    case 6:
        return CallProc<Ret, long long, double, double>(proc, slots[0].integer_value, slots[1].double_value, slots[2].double_value);
    default:
        return CallProc<Ret, double, double, double>(proc, slots[0].double_value, slots[1].double_value, slots[2].double_value);
    }
}

template <typename Ret>
Ret Invoke4(FARPROC proc, const Slot* slots)
{
    const int mask = (slots[0].kind == SlotKind::Double ? 1 : 0)
        | (slots[1].kind == SlotKind::Double ? 2 : 0)
        | (slots[2].kind == SlotKind::Double ? 4 : 0)
        | (slots[3].kind == SlotKind::Double ? 8 : 0);

    switch (mask)
    {
    case 0:
        return CallProc<Ret, long long, long long, long long, long long>(proc, slots[0].integer_value, slots[1].integer_value, slots[2].integer_value, slots[3].integer_value);
    case 1:
        return CallProc<Ret, double, long long, long long, long long>(proc, slots[0].double_value, slots[1].integer_value, slots[2].integer_value, slots[3].integer_value);
    case 2:
        return CallProc<Ret, long long, double, long long, long long>(proc, slots[0].integer_value, slots[1].double_value, slots[2].integer_value, slots[3].integer_value);
    case 3:
        return CallProc<Ret, double, double, long long, long long>(proc, slots[0].double_value, slots[1].double_value, slots[2].integer_value, slots[3].integer_value);
    case 4:
        return CallProc<Ret, long long, long long, double, long long>(proc, slots[0].integer_value, slots[1].integer_value, slots[2].double_value, slots[3].integer_value);
    case 5:
        return CallProc<Ret, double, long long, double, long long>(proc, slots[0].double_value, slots[1].integer_value, slots[2].double_value, slots[3].integer_value);
    case 6:
        return CallProc<Ret, long long, double, double, long long>(proc, slots[0].integer_value, slots[1].double_value, slots[2].double_value, slots[3].integer_value);
    case 7:
        return CallProc<Ret, double, double, double, long long>(proc, slots[0].double_value, slots[1].double_value, slots[2].double_value, slots[3].integer_value);
    case 8:
        return CallProc<Ret, long long, long long, long long, double>(proc, slots[0].integer_value, slots[1].integer_value, slots[2].integer_value, slots[3].double_value);
    case 9:
        return CallProc<Ret, double, long long, long long, double>(proc, slots[0].double_value, slots[1].integer_value, slots[2].integer_value, slots[3].double_value);
    case 10:
        return CallProc<Ret, long long, double, long long, double>(proc, slots[0].integer_value, slots[1].double_value, slots[2].integer_value, slots[3].double_value);
    case 11:
        return CallProc<Ret, double, double, long long, double>(proc, slots[0].double_value, slots[1].double_value, slots[2].integer_value, slots[3].double_value);
    case 12:
        return CallProc<Ret, long long, long long, double, double>(proc, slots[0].integer_value, slots[1].integer_value, slots[2].double_value, slots[3].double_value);
    case 13:
        return CallProc<Ret, double, long long, double, double>(proc, slots[0].double_value, slots[1].integer_value, slots[2].double_value, slots[3].double_value);
    case 14:
        return CallProc<Ret, long long, double, double, double>(proc, slots[0].integer_value, slots[1].double_value, slots[2].double_value, slots[3].double_value);
    default:
        return CallProc<Ret, double, double, double, double>(proc, slots[0].double_value, slots[1].double_value, slots[2].double_value, slots[3].double_value);
    }
}

template <typename Ret>
Ret Invoke(FARPROC proc, const Slot* slots, size_t count)
{
    switch (count)
    {
    case 0:
        return CallProc<Ret>(proc);
    case 1:
        if (slots[0].kind == SlotKind::Double)
            return CallProc<Ret, double>(proc, slots[0].double_value);
        return CallProc<Ret, long long>(proc, slots[0].integer_value);
    case 2:
        return Invoke2<Ret>(proc, slots);
    case 3:
        return Invoke3<Ret>(proc, slots);
    case 4:
        return Invoke4<Ret>(proc, slots);
    default:
        return CallProc<Ret>(proc);
    }
}

int DllHandleGc(lua_State* L)
{
    DllHandle* handle = static_cast<DllHandle*>(lua_touserdata(L, 1));
    if (handle != nullptr)
    {
        if (handle->module != nullptr)
        {
            FreeLibrary(handle->module);
            handle->module = nullptr;
        }
        handle->~DllHandle();
    }
    return 0;
}

int DllFunctionGc(lua_State* L)
{
    DllFunction* function = static_cast<DllFunction*>(lua_touserdata(L, 1));
    if (function != nullptr)
        function->~DllFunction();
    return 0;
}

DllHandle* CheckHandle(lua_State* L, int index)
{
    return static_cast<DllHandle*>(luaL_checkudata(L, index, kHandleMetatable));
}

Slot ReadSlot(lua_State* L, int index, ValueKind kind)
{
    Slot slot;
    if (kind == ValueKind::Double)
    {
        slot.kind = SlotKind::Double;
        slot.double_value = luaL_checknumber(L, index);
        return slot;
    }

    slot.kind = SlotKind::Integer;
    switch (kind)
    {
    case ValueKind::Pointer:
        if (lua_islightuserdata(L, index))
            slot.integer_value = static_cast<long long>(reinterpret_cast<intptr_t>(lua_touserdata(L, index)));
        else if (lua_isstring(L, index))
            slot.integer_value = static_cast<long long>(reinterpret_cast<intptr_t>(lua_tolstring(L, index, nullptr)));
        else if (lua_isnil(L, index))
            slot.integer_value = 0;
        else
            slot.integer_value = static_cast<long long>(luaL_checkinteger(L, index));
        break;
    case ValueKind::Boolean:
        slot.integer_value = lua_toboolean(L, index);
        break;
    default:
        slot.integer_value = static_cast<long long>(luaL_checkinteger(L, index));
        break;
    }
    return slot;
}

int InvokeAndPush(lua_State* L, const DllFunction& function, const Slot* slots)
{
    const size_t count = function.arg_kinds.size();
    switch (function.return_kind)
    {
    case ValueKind::Void:
        Invoke<void>(function.proc, slots, count);
        lua_pushnil(L);
        return 1;
    case ValueKind::Int32:
        lua_pushinteger(L, Invoke<int32_t>(function.proc, slots, count));
        return 1;
    case ValueKind::UInt32:
        lua_pushinteger(L, Invoke<uint32_t>(function.proc, slots, count));
        return 1;
    case ValueKind::Int64:
        lua_pushinteger(L, Invoke<int64_t>(function.proc, slots, count));
        return 1;
    case ValueKind::UInt64:
        lua_pushinteger(L, static_cast<lua_Integer>(Invoke<uint64_t>(function.proc, slots, count)));
        return 1;
    case ValueKind::Double:
        lua_pushnumber(L, Invoke<double>(function.proc, slots, count));
        return 1;
    case ValueKind::Pointer:
        lua_pushlightuserdata(L, reinterpret_cast<void*>(Invoke<intptr_t>(function.proc, slots, count)));
        return 1;
    case ValueKind::Boolean:
        lua_pushboolean(L, Invoke<int32_t>(function.proc, slots, count) != 0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

int DllLoad(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);
    const std::wstring wide_path = Utf8ToWide(path);
    HMODULE module = LoadLibraryW(wide_path.c_str());
    if (module == nullptr)
    {
        lua_pushnil(L);
        lua_pushfstring(L, "dll.load failed: %s (error %d)", path, static_cast<int>(GetLastError()));
        return 2;
    }

    DllHandle* handle = new (lua_newuserdatauv(L, sizeof(DllHandle), 0)) DllHandle;
    handle->module = module;
    luaL_setmetatable(L, kHandleMetatable);
    return 1;
}

int DllBind(lua_State* L)
{
    DllHandle* handle = CheckHandle(L, 1);
    const char* symbol = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);

    if (handle->module == nullptr)
        return luaL_error(L, "dll.bind: library is closed");

    DllFunction* function = new (lua_newuserdatauv(L, sizeof(DllFunction), 0)) DllFunction;
    luaL_setmetatable(L, kFunctionMetatable);

    function->proc = GetProcAddress(handle->module, symbol);
    if (function->proc == nullptr)
    {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushfstring(L, "dll.bind failed: %s", symbol);
        return 2;
    }

    lua_getfield(L, 3, "ret");
    if (!lua_isnil(L, -1) && !ParseValueKind(luaL_checkstring(L, -1), function->return_kind))
        return luaL_error(L, "dll.bind: unsupported return type");
    lua_pop(L, 1);

    lua_getfield(L, 3, "args");
    if (!lua_isnil(L, -1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);
        const size_t count = lua_rawlen(L, -1);
        if (count > static_cast<size_t>(kMaxArguments))
            return luaL_error(L, "dll.bind: at most %d arguments are supported", kMaxArguments);

        function->arg_kinds.reserve(count);
        for (size_t index = 1; index <= count; ++index)
        {
            lua_rawgeti(L, -1, static_cast<lua_Integer>(index));
            ValueKind kind = ValueKind::Void;
            if (!ParseValueKind(luaL_checkstring(L, -1), kind) || kind == ValueKind::Void)
                return luaL_error(L, "dll.bind: unsupported argument type");
            function->arg_kinds.push_back(kind);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    lua_pushvalue(L, 1);
    lua_pushvalue(L, -2);
    lua_pushcclosure(L, [](lua_State* state) -> int
    {
        DllHandle* function_handle = static_cast<DllHandle*>(lua_touserdata(state, lua_upvalueindex(1)));
        DllFunction* bound_function = static_cast<DllFunction*>(lua_touserdata(state, lua_upvalueindex(2)));

        if (function_handle == nullptr || function_handle->module == nullptr)
            return luaL_error(state, "dll function: library is closed");
        if (bound_function == nullptr)
            return luaL_error(state, "dll function: invalid function");

        const int expected = static_cast<int>(bound_function->arg_kinds.size());
        const int provided = lua_gettop(state);
        if (provided != expected)
            return luaL_error(state, "dll function: expected %d arguments, got %d", expected, provided);

        Slot slots[kMaxArguments];
        for (int index = 0; index < expected; ++index)
            slots[index] = ReadSlot(state, index + 1, bound_function->arg_kinds[static_cast<size_t>(index)]);

        return InvokeAndPush(state, *bound_function, slots);
    }, 2);
    return 1;
}

int DllClose(lua_State* L)
{
    DllHandle* handle = CheckHandle(L, 1);
    if (handle->module != nullptr)
    {
        FreeLibrary(handle->module);
        handle->module = nullptr;
    }
    return 0;
}
}

void RegisterLuaDllApi(lua_State* L)
{
    if (luaL_newmetatable(L, kHandleMetatable))
    {
        lua_pushcfunction(L, DllHandleGc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);

    if (luaL_newmetatable(L, kFunctionMetatable))
    {
        lua_pushcfunction(L, DllFunctionGc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);

    lua_newtable(L);
    lua_pushcfunction(L, DllLoad);
    lua_setfield(L, -2, "load");
    lua_pushcfunction(L, DllBind);
    lua_setfield(L, -2, "bind");
    lua_pushcfunction(L, DllClose);
    lua_setfield(L, -2, "close");
    lua_setglobal(L, "dll");
}