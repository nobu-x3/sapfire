#pragma once

// Compatibility header for transitioning from old d3d:: namespace to new sf::render:: namespace
// This provides type aliases to minimize breaking changes during refactor

#include "render/render_api.h"
#include "render/resource_types.h"
#include "render/i_graphics_device.h"

namespace Sapfire::d3d {

// Type aliases for backward compatibility
using Buffer = sf::render::Buffer;
using Texture = sf::render::Texture;
using BufferUsage = sf::render::BufferUsage;
using TextureUsage = sf::render::TextureUsage;
using BufferCreationDesc = sf::render::BufferCreationDesc;
using TextureCreationDesc = sf::render::TextureCreationDesc;
using GraphicsPipelineStateCreationDesc = sf::render::GraphicsPipelineStateDesc;
using ComputePipelineStateCreationDesc = sf::render::ComputePipelineStateDesc;
using SwapchainCreationDesc = sf::render::SwapchainCreationDesc;

// Constants
constexpr u32 MAX_FRAMES_IN_FLIGHT = 3;
constexpr u32 INVALID_INDEX_U32 = sf::render::INVALID_DESCRIPTOR_INDEX;

// Shader types (for shader compilation)
enum class ShaderType : u8 {
    Vertex,
    Pixel,
    Compute,
    RootSignature
};

// Helper functions from dx12_util
inline std::wstring AnsiToWString(const std::string& str) {
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

inline std::string WStringToANSI(const std::wstring_view input_wstring) {
    std::string result{};
    const std::wstring input{input_wstring};
    const int32_t length = ::WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, NULL, 0, NULL, NULL);
    if (length > 0) {
        result.resize(size_t(length) - 1);
        WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, result.data(), length, NULL, NULL);
    }
    return result;
}

// Legacy GraphicsDevice wrapper for compatibility
// This wraps the new abstract IGraphicsDevice interface
class GraphicsDevice {
public:
    explicit GraphicsDevice(const SwapchainCreationDesc& desc);
    ~GraphicsDevice() = default;

    // Forward all calls to the abstract interface
    void begin_frame() { m_Device->begin_frame(); }
    void end_frame() { m_Device->end_frame(); }
    void present() { m_Device->present(); }
    void resize_window(u32 width, u32 height) { m_Device->resize_window(width, height); }

    Texture& current_back_buffer() { return m_Device->get_current_back_buffer(); }

    template<typename T>
    Buffer create_buffer(const BufferCreationDesc& desc, stl::span<const T> data = {}) {
        if (data.empty()) {
            return m_Device->create_buffer(desc);
        } else {
            return m_Device->create_buffer(desc, data);
        }
    }

    Texture create_texture(const TextureCreationDesc& desc, const void* data = nullptr) {
        if (data) {
            // Note: size calculation would need proper implementation
            return m_Device->create_texture_with_data(desc, data, desc.width * desc.height * 4);
        } else {
            return m_Device->create_texture(desc);
        }
    }

    // Get underlying abstract device for new code
    sf::render::IGraphicsDevice* device() const { return m_Device; }
    sf::render::IGraphicsDevice* operator->() { return m_Device; }

private:
    sf::render::IGraphicsDevice* m_Device = nullptr;
};

} // namespace Sapfire::d3d
