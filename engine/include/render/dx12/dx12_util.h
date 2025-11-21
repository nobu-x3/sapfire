#pragma once

#include "core/base.h"
#include <d3d12.h>
#include <comdef.h>
#include <string>

namespace sf::render::dx12 {

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

// ============================================================================
// String Conversion Utilities
// ============================================================================

inline std::wstring ansi_to_wstring(const std::string& str) {
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

inline std::string wstring_to_ansi(const std::wstring_view input_wstring) {
    std::string result{};
    const std::wstring input{input_wstring};
    const int32_t length = ::WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, NULL, 0, NULL, NULL);
    if (length > 0) {
        result.resize(size_t(length) - 1);
        WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, result.data(), length, NULL, NULL);
    }
    return result;
}

// ============================================================================
// DX12 Exception Handling
// ============================================================================

class DxException {
public:
    DxException() = default;
    DxException(HRESULT hr, const std::wstring& function_name, const std::wstring& filename, int line_number)
        : error_code(hr), function_name(function_name), filename(filename), line_number(line_number) {}

    std::wstring to_string() const {
        _com_error err(error_code);
#ifdef UNICODE
        std::wstring msg = err.ErrorMessage();
        return function_name + L" failed in " + filename + L"; line " + std::to_wstring(line_number) + L"; error: " + msg;
#else
        std::wstring msg = ansi_to_wstring(err.ErrorMessage());
        return function_name + L" failed in " + filename + L"; line " + std::to_wstring(line_number) + L"; error: " + msg;
#endif
    }

    HRESULT error_code = S_OK;
    std::wstring function_name;
    std::wstring filename;
    int line_number = -1;
};

#ifndef dx12_check
#define dx12_check(x)                                                           \
    {                                                                           \
        const HRESULT hr__ = (x);                                              \
        const std::wstring wfn = sf::render::dx12::ansi_to_wstring(__FILE__); \
        if (FAILED(hr__)) {                                                    \
            __debugbreak();                                                    \
            throw sf::render::dx12::DxException(hr__, L#x, wfn, __LINE__);    \
        }                                                                       \
    }
#endif

// ============================================================================
// Utility Functions
// ============================================================================

inline u32 calculate_constant_buffer_byte_size(u32 byte_size) {
    return (byte_size + 255) & ~255;
}

} // namespace sf::render::dx12
