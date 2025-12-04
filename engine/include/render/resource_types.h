#pragma once

#include <span>
#include "core/core.h"
#include "render_api.h"

namespace sf::render {

    // Opaque Backend Handles

    // These handles hide backend-specific resource pointers
    // DX12: ID3D12Resource* + D3D12MA::Allocation*
    // Vulkan: VkBuffer/VkImage + VmaAllocation
    using ResourceHandle = void*;
    using AllocationHandle = void*;

    constexpr u32 INVALID_DESCRIPTOR_INDEX = 0xFFFFFFFF;

    // Buffer Types

    enum class BufferUsage : u8 {
        Upload, // CPU -> GPU upload buffer (staging)
        Download, // GPU -> CPU download buffer (readback)
        Vertex, // Vertex buffer
        Index, // Index buffer
        Uniform, // Uniform/Constant buffer (CBV)
        Storage, // Storage buffer (SRV/UAV)
        Structured, // Structured buffer (SRV/UAV) - alias for Storage
        Constant, // Constant buffer (CBV) - alias for Uniform
        Staging, // Staging buffer - alias for Upload
    };

    // Texture Usage

    enum class TextureUsage : u8 {
        DepthStencil,
        RenderTarget,
        ShaderResource,
        UnorderedAccess,
        CubeMap,
    };

    // Resource Creation Descriptors

    struct BufferCreationDesc {
        BufferUsage usage = BufferUsage::Structured;
        u64 size_in_bytes = 0;
        bool should_map = false;
        stl::string_view name = {};
        ResourceUsage resource_usage = ResourceUsage::None; // Additional usage flags
    };

    struct TextureCreationDesc {
        TextureUsage usage = TextureUsage::ShaderResource;
        TextureType type = TextureType::Texture2D;
        Format format = Format::RGBA8_UNORM;
        u32 width = 1;
        u32 height = 1;
        u32 depth_or_array_size = 1;
        u32 mip_levels = 1;
        ResourceState initial_state = ResourceState::Undefined;
        ResourceUsage resource_usage = ResourceUsage::None;
        stl::string_view name = {};
        std::string path; // For loading from file
    };

    // Descriptor View Creation

    struct ShaderResourceViewDesc {
        Format format = Format::Unknown;
        TextureType view_dimension = TextureType::Texture2D;
        u32 most_detailed_mip = 0;
        u32 mip_levels = 1;
        u32 first_array_slice = 0;
        u32 array_size = 1;
    };

    struct UnorderedAccessViewDesc {
        Format format = Format::Unknown;
        TextureType view_dimension = TextureType::Texture2D;
        u32 mip_slice = 0;
        u32 first_array_slice = 0;
        u32 array_size = 1;
    };

    struct RenderTargetViewDesc {
        Format format = Format::Unknown;
        TextureType view_dimension = TextureType::Texture2D;
        u32 mip_slice = 0;
        u32 first_array_slice = 0;
        u32 array_size = 1;
    };

    struct DepthStencilViewDesc {
        Format format = Format::Unknown;
        TextureType view_dimension = TextureType::Texture2D;
        u32 mip_slice = 0;
        u32 first_array_slice = 0;
        u32 array_size = 1;
    };

    struct ConstantBufferViewDesc {
        u64 buffer_location = 0;
        u32 size_in_bytes = 0;
    };

    // Buffer Structure

    struct Buffer {
        // Opaque backend handles (hidden from user)
        ResourceHandle resource = nullptr;
        AllocationHandle allocation = nullptr;
        void* mapped_data = nullptr; // For upload buffers

        // Public data
        u64 size_in_bytes = 0;

        // Descriptor indices (exposed to user for bindless)
        u32 srv_index = INVALID_DESCRIPTOR_INDEX;
        u32 uav_index = INVALID_DESCRIPTOR_INDEX;
        u32 cbv_index = INVALID_DESCRIPTOR_INDEX;

        // Helper method to update constant buffer data
        void update(const void* data, size_t size);
    };

    // Texture Structure

    struct Texture {
        // Opaque backend handles (hidden from user)
        ResourceHandle resource = nullptr; // VkImage / ID3D12Resource
        ResourceHandle image_view = nullptr; // VkImageView (Vulkan only, nullptr on DX12)
        AllocationHandle allocation = nullptr;

        // Public data
        u32 width = 0;
        u32 height = 0;
        u32 depth_or_array_size = 1;
        u32 mip_levels = 1;
        Format format = Format::Unknown;
        TextureType type = TextureType::Texture2D;

        // Descriptor indices (exposed to user for bindless)
        u32 srv_index = INVALID_DESCRIPTOR_INDEX;
        u32 uav_index = INVALID_DESCRIPTOR_INDEX;
        u32 rtv_index = INVALID_DESCRIPTOR_INDEX;
        u32 dsv_index = INVALID_DESCRIPTOR_INDEX;
    };

    // Shader Module

    struct ShaderModule {
        stl::string_view path = {};
        stl::string_view entry_point = "main";
    };

    // Pipeline State Creation

    struct GraphicsPipelineDesc {
        // Pipeline layout (must be set)
        class IPipelineLayout* layout = nullptr;

        // Render pass (must be set)
        class IRenderPass* render_pass = nullptr;
        u32 subpass = 0;

        // Shader stages
        ShaderModule vertex_shader = {};
        ShaderModule pixel_shader = {};

        // Fixed-function state
        PrimitiveTopology topology = PrimitiveTopology::TriangleList;
        RasterizerState rasterizer = {};
        DepthStencilState depth_stencil = {};
        BlendState blend_state = {};

        stl::string_view name = {};
    };

    struct ComputePipelineDesc {
        // Pipeline layout (must be set)
        class IPipelineLayout* layout = nullptr;

        // Shader stage
        ShaderModule compute_shader = {};

        stl::string_view name = {};
    };

    // Swapchain Creation

    struct SwapchainCreationDesc {
        void* window_handle = nullptr; // Native window handle (HWND on Windows, X11 Window on Linux, etc.)
        u32 width = 1920;
        u32 height = 1080;
        u32 buffer_count = 3;
        Format format = Format::RGBA16_FLOAT;
        u32 refresh_rate = 60;
        bool headless = false; // If true, create offscreen render targets instead of swapchain (for editor viewports)
    };

} // namespace sf::render
