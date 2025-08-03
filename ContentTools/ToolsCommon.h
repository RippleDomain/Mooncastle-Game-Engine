#pragma once

#include "CommonHeaders.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <wrl.h>

#ifndef  EDITOR_INTERFACE
#define EDITOR_INTERFACE extern "C" __declspec(dllexport)
#endif

class progression
{
public:
    using progressCallback = void(*)(i32, i32);

    progression() = default;

    explicit progression(progressCallback newCallback) : callback{ newCallback }
    {
    }

    DISABLE_COPY(progression);

    void setCallback(u32 newValue, u32 newMaxValue)
    {
        value = newValue;
        maxValue = newMaxValue;
        if (callback) callback(newValue, newMaxValue);
    }

    [[nodiscard]] constexpr u32 getMaxValue() const { return maxValue; }
    [[nodiscard]] constexpr u32 getValue() const { return value; }

private:
    progressCallback    callback{ nullptr };
    u32                 value{ 0 };
    u32                 maxValue{ 0 };
};

inline bool fileExists(const char* file)
{
    const DWORD attr{ GetFileAttributesA(file) };
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

inline std::wstring toWString(const char* cstr)
{
    std::string s(cstr);
    return { s.begin(), s.end() };
}

inline mooncastle::utl::vector<std::string> split(std::string s, char delimiter)
{
    size_t start{ 0 };
    size_t end{ 0 };
    std::string substring;

    mooncastle::utl::vector<std::string> strings;

    while ((end = s.find(delimiter, start)) != std::string::npos)
    {
        substring = s.substr(start, end - start);
        start = end + sizeof(char);
        strings.emplace_back(substring);
    }

    strings.emplace_back(s.substr(start));

    return strings;
}