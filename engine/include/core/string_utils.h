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
// Linux/macOS: Use standard library string conversion
#include <codecvt>
#include <locale>

inline std::wstring to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

inline std::string to_string(const std::wstring_view wstr) {
    if (wstr.empty()) return std::string();
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr.data(), wstr.data() + wstr.size());
}
#endif

} // namespace sf::string_utils
