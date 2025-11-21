#pragma once

#include "core/core.h"
#include <string>

namespace sf::string_utils {

// Cross-platform string conversion utilities

#ifdef SF_PLATFORM_WINDOWS
// Windows: Use WinAPI for string conversion
inline std::wstring to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

inline std::string to_string(const std::wstring_view wstr) {
    if (wstr.empty()) return std::string();
    std::string result{};
    const std::wstring input{wstr};
    const int32_t length = ::WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, NULL, 0, NULL, NULL);
    if (length > 0) {
        result.resize(size_t(length) - 1);
        WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, result.data(), length, NULL, NULL);
    }
    return result;
}
#else
// Linux/macOS: Use mbstowcs/wcstombs for string conversion
#include <cstdlib>
#include <cwchar>

inline std::wstring to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    size_t size_needed = mbstowcs(nullptr, str.c_str(), 0) + 1;
    std::wstring result(size_needed, 0);
    mbstowcs(&result[0], str.c_str(), size_needed);
    result.resize(size_needed - 1); // Remove null terminator
    return result;
}

inline std::string to_string(const std::wstring_view wstr) {
    if (wstr.empty()) return std::string();
    size_t size_needed = wcstombs(nullptr, std::wstring(wstr).c_str(), 0) + 1;
    std::string result(size_needed, 0);
    wcstombs(&result[0], std::wstring(wstr).c_str(), size_needed);
    result.resize(size_needed - 1); // Remove null terminator
    return result;
}
#endif

} // namespace sf::string_utils
