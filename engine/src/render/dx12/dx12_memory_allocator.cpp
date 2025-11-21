#include "engpch.h"
#include "render/dx12/dx12_memory_allocator.h"
#include "render/dx12/dx12_type_conversions.h"
#include "render/dx12/dx12_util.h"
#include <d3d12.h>

namespace sf::render::dx12 {

DX12MemoryAllocator::DX12MemoryAllocator(ID3D12Device* device, IDXGIAdapter* adapter)
    : m_Device(device) {

    D3D12MA::ALLOCATOR_DESC desc{
        .pDevice = device,
        .pAdapter = adapter,
    };
    dx12_check(D3D12MA::CreateAllocator(&desc, &m_Allocator));
}

DX12MemoryAllocator::~DX12MemoryAllocator() {
    // D3D12MA::Allocator is a ComPtr, will release automatically
}

void DX12MemoryAllocator::allocate_buffer(Buffer& buffer, const BufferCreationDesc& desc) {
    D3D12_RESOURCE_STATES resource_state{};
    D3D12_HEAP_TYPE heap_type{};
    bool is_cpu_visible = false;

    switch (desc.usage) {
        case BufferUsage::Upload:
        case BufferUsage::Constant:
            resource_state = D3D12_RESOURCE_STATE_GENERIC_READ;
            heap_type = D3D12_HEAP_TYPE_UPLOAD;
            is_cpu_visible = true;
            break;
        case BufferUsage::Index:
        case BufferUsage::Structured:
            resource_state = D3D12_RESOURCE_STATE_COMMON;
            heap_type = D3D12_HEAP_TYPE_DEFAULT;
            is_cpu_visible = false;
            break;
    }

    D3D12_RESOURCE_DESC resource_desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = desc.size_in_bytes,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = { .Count = 1, .Quality = 0 },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE
    };

    // Allow UAV if requested
    if (has_flag(desc.resource_usage, ResourceUsage::UnorderedAccess)) {
        resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    D3D12MA::ALLOCATION_DESC allocation_desc{
        .HeapType = heap_type,
    };

    D3D12MA::Allocation* allocation = nullptr;
    ID3D12Resource* resource = nullptr;

    dx12_check(m_Allocator->CreateResource(
        &allocation_desc,
        &resource_desc,
        resource_state,
        nullptr,
        &allocation,
        IID_PPV_ARGS(&resource)
    ));

    // Store in buffer
    buffer.resource = resource;
    buffer.allocation = allocation;
    buffer.size_in_bytes = desc.size_in_bytes;

    // Map if CPU visible
    if (is_cpu_visible) {
        dx12_check(resource->Map(0, nullptr, &buffer.mapped_data));
    }

    // Set name
    if (!desc.name.empty()) {
        resource->SetName(desc.name.data());
    }

    allocation->SetResource(resource);
}

void DX12MemoryAllocator::allocate_texture(Texture& texture, const TextureCreationDesc& desc) {
    DXGI_FORMAT format = to_dxgi_format(desc.format);
    DXGI_FORMAT ds_format = format;

    // Handle depth formats
    if (desc.format == Format::D32_FLOAT) {
        ds_format = DXGI_FORMAT_D32_FLOAT;
        format = DXGI_FORMAT_R32_FLOAT;
    } else if (desc.format == Format::D24_UNORM_S8_UINT) {
        ds_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    }

    D3D12_RESOURCE_DESC resource_desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
        .Width = desc.width,
        .Height = desc.height,
        .DepthOrArraySize = static_cast<UINT16>(desc.depth_or_array_size),
        .MipLevels = static_cast<UINT16>(desc.mip_levels),
        .Format = format,
        .SampleDesc = { .Count = 1, .Quality = 0 },
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_NONE
    };

    // Clamp mip levels
    if (resource_desc.MipLevels >= resource_desc.Width) {
        resource_desc.MipLevels = static_cast<UINT16>(resource_desc.Width - 1);
    }
    if (resource_desc.MipLevels >= resource_desc.Height) {
        resource_desc.MipLevels = static_cast<UINT16>(resource_desc.Height - 1);
    }

    D3D12_RESOURCE_STATES resource_state = to_d3d12_resource_state(desc.initial_state);
    D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12MA::ALLOCATION_DESC allocation_desc{
        .HeapType = heap_type,
    };

    // Handle different texture usages
    switch (desc.usage) {
        case TextureUsage::DepthStencil:
            resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            resource_desc.Format = ds_format;
            allocation_desc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
            resource_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            break;

        case TextureUsage::RenderTarget:
            resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            allocation_desc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
            allocation_desc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
            if (desc.initial_state == ResourceState::Common) {
                resource_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
            }
            break;

        case TextureUsage::UnorderedAccess:
        case TextureUsage::ShaderResource:
        case TextureUsage::CubeMap:
            resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            break;
    }

    // Check usage flags
    if (has_flag(desc.resource_usage, ResourceUsage::UnorderedAccess)) {
        resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if (has_flag(desc.resource_usage, ResourceUsage::RenderTarget)) {
        resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (has_flag(desc.resource_usage, ResourceUsage::DepthStencil)) {
        resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }

    // Optimized clear value
    stl::optional<D3D12_CLEAR_VALUE> clear_value{};
    if (desc.usage == TextureUsage::RenderTarget) {
        clear_value = D3D12_CLEAR_VALUE{
            .Format = format,
            .Color = {0.0f, 0.0f, 0.0f, 1.0f}
        };
    } else if (desc.usage == TextureUsage::DepthStencil) {
        clear_value = D3D12_CLEAR_VALUE{
            .Format = ds_format,
            .DepthStencil = { .Depth = 1.0f, .Stencil = 0 }
        };
    }

    D3D12MA::Allocation* allocation = nullptr;
    ID3D12Resource* resource = nullptr;

    dx12_check(m_Allocator->CreateResource(
        &allocation_desc,
        &resource_desc,
        resource_state,
        clear_value.has_value() ? &clear_value.value() : nullptr,
        &allocation,
        IID_PPV_ARGS(&resource)
    ));

    // Store in texture
    texture.resource = resource;
    texture.allocation = allocation;
    texture.width = desc.width;
    texture.height = desc.height;
    texture.depth_or_array_size = desc.depth_or_array_size;
    texture.mip_levels = desc.mip_levels;
    texture.format = desc.format;

    // Set name
    if (!desc.name.empty()) {
        resource->SetName(desc.name.data());
    }

    allocation->SetResource(resource);
}

void DX12MemoryAllocator::free_buffer(Buffer& buffer) {
    if (buffer.allocation) {
        D3D12MA::Allocation* alloc = static_cast<D3D12MA::Allocation*>(buffer.allocation);
        alloc->Release();
        buffer.allocation = nullptr;
        buffer.resource = nullptr;
        buffer.mapped_data = nullptr;
    }
}

void DX12MemoryAllocator::free_texture(Texture& texture) {
    if (texture.allocation) {
        D3D12MA::Allocation* alloc = static_cast<D3D12MA::Allocation*>(texture.allocation);
        alloc->Release();
        texture.allocation = nullptr;
        texture.resource = nullptr;
    }
}

void DX12MemoryAllocator::get_stats(void* stats_out) {
    if (stats_out) {
        D3D12MA::TotalStatistics* stats = static_cast<D3D12MA::TotalStatistics*>(stats_out);
        m_Allocator->CalculateStatistics(stats);
    }
}

} // namespace sf::render::dx12
