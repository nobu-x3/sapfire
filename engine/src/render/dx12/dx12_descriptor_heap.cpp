#include "engpch.h"
#include "render/dx12/dx12_descriptor_heap.h"
#include "render/dx12/dx12_type_conversions.h"
#include "render/dx12/dx12_util.h"

namespace sf::render::dx12 {

DX12DescriptorHeap::DX12DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heap_type, u32 descriptor_count, const wchar_t* name)
    : m_Device(device), m_HeapType(heap_type), m_DescriptorCount(descriptor_count) {

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {
        .Type = heap_type,
        .NumDescriptors = descriptor_count,
        .Flags = (heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
                     ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                     : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask = 0
    };

    dx12_check(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&m_DescriptorHeap)));
    m_DescriptorHeap->SetName(name);

    m_DescriptorSize = device->GetDescriptorHandleIncrementSize(heap_type);
    m_CPUStart = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    if (heap_desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
        m_GPUStart = m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    }
}

u32 DX12DescriptorHeap::allocate_srv(Buffer& buffer) {
    u32 index = m_CurrentIndex++;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Buffer = {
            .FirstElement = 0,
            .NumElements = static_cast<UINT>(buffer.size_in_bytes / sizeof(u32)),
            .StructureByteStride = sizeof(u32),
            .Flags = D3D12_BUFFER_SRV_FLAG_NONE
        }
    };

    D3D12_CPU_DESCRIPTOR_HANDLE handle = get_cpu_descriptor_handle(index);
    m_Device->CreateShaderResourceView(static_cast<ID3D12Resource*>(buffer.resource), &srv_desc, handle);

    return index;
}

u32 DX12DescriptorHeap::allocate_srv(Texture& texture, const ShaderResourceViewDesc* desc) {
    u32 index = m_CurrentIndex++;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
        .Format = desc ? to_dxgi_format(desc->format) : to_dxgi_format(texture.format),
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MostDetailedMip = desc ? desc->most_detailed_mip : 0u,
            .MipLevels = desc ? desc->mip_levels : texture.mip_levels,
            .PlaneSlice = 0,
            .ResourceMinLODClamp = 0.0f
        }
    };

    D3D12_CPU_DESCRIPTOR_HANDLE handle = get_cpu_descriptor_handle(index);
    m_Device->CreateShaderResourceView(static_cast<ID3D12Resource*>(texture.resource), &srv_desc, handle);

    return index;
}

u32 DX12DescriptorHeap::allocate_uav(Buffer& buffer) {
    u32 index = m_CurrentIndex++;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
        .Buffer = {
            .FirstElement = 0,
            .NumElements = static_cast<UINT>(buffer.size_in_bytes / sizeof(u32)),
            .StructureByteStride = sizeof(u32),
            .CounterOffsetInBytes = 0,
            .Flags = D3D12_BUFFER_UAV_FLAG_NONE
        }
    };

    D3D12_CPU_DESCRIPTOR_HANDLE handle = get_cpu_descriptor_handle(index);
    m_Device->CreateUnorderedAccessView(static_cast<ID3D12Resource*>(buffer.resource), nullptr, &uav_desc, handle);

    return index;
}

u32 DX12DescriptorHeap::allocate_uav(Texture& texture, const UnorderedAccessViewDesc* desc) {
    u32 index = m_CurrentIndex++;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {
        .Format = desc ? to_dxgi_format(desc->format) : to_dxgi_format(texture.format),
        .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
        .Texture2D = {
            .MipSlice = desc ? desc->mip_slice : 0u,
            .PlaneSlice = 0
        }
    };

    D3D12_CPU_DESCRIPTOR_HANDLE handle = get_cpu_descriptor_handle(index);
    m_Device->CreateUnorderedAccessView(static_cast<ID3D12Resource*>(texture.resource), nullptr, &uav_desc, handle);

    return index;
}

u32 DX12DescriptorHeap::allocate_cbv(Buffer& buffer) {
    u32 index = m_CurrentIndex++;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {
        .BufferLocation = static_cast<ID3D12Resource*>(buffer.resource)->GetGPUVirtualAddress(),
        .SizeInBytes = calculate_constant_buffer_byte_size(static_cast<u32>(buffer.size_in_bytes))
    };

    D3D12_CPU_DESCRIPTOR_HANDLE handle = get_cpu_descriptor_handle(index);
    m_Device->CreateConstantBufferView(&cbv_desc, handle);

    return index;
}

u32 DX12DescriptorHeap::allocate_cbv(const ConstantBufferViewDesc& desc) {
    u32 index = m_CurrentIndex++;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {
        .BufferLocation = desc.buffer_location,
        .SizeInBytes = calculate_constant_buffer_byte_size(desc.size_in_bytes)
    };

    D3D12_CPU_DESCRIPTOR_HANDLE handle = get_cpu_descriptor_handle(index);
    m_Device->CreateConstantBufferView(&cbv_desc, handle);

    return index;
}

u32 DX12DescriptorHeap::allocate_rtv(Texture& texture, const RenderTargetViewDesc* desc) {
    u32 index = m_CurrentIndex++;

    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
        .Format = desc ? to_dxgi_format(desc->format) : to_dxgi_format(texture.format),
        .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        .Texture2D = {
            .MipSlice = desc ? desc->mip_slice : 0u,
            .PlaneSlice = 0
        }
    };

    D3D12_CPU_DESCRIPTOR_HANDLE handle = get_cpu_descriptor_handle(index);
    m_Device->CreateRenderTargetView(static_cast<ID3D12Resource*>(texture.resource), &rtv_desc, handle);

    return index;
}

u32 DX12DescriptorHeap::allocate_dsv(Texture& texture, const DepthStencilViewDesc* desc) {
    u32 index = m_CurrentIndex++;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
        .Format = desc ? to_dxgi_format(desc->format) : to_dxgi_format(texture.format),
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags = D3D12_DSV_FLAG_NONE,
        .Texture2D = {
            .MipSlice = desc ? desc->mip_slice : 0u
        }
    };

    D3D12_CPU_DESCRIPTOR_HANDLE handle = get_cpu_descriptor_handle(index);
    m_Device->CreateDepthStencilView(static_cast<ID3D12Resource*>(texture.resource), &dsv_desc, handle);

    return index;
}

u32 DX12DescriptorHeap::allocate_sampler(const SamplerDesc& desc) {
    u32 index = m_CurrentIndex++;

    D3D12_SAMPLER_DESC sampler_desc = {
        .Filter = to_d3d12_filter(desc.filter),
        .AddressU = to_d3d12_texture_address_mode(desc.address_u),
        .AddressV = to_d3d12_texture_address_mode(desc.address_v),
        .AddressW = to_d3d12_texture_address_mode(desc.address_w),
        .MipLODBias = desc.mip_lod_bias,
        .MaxAnisotropy = desc.max_anisotropy,
        .ComparisonFunc = to_d3d12_comparison_func(desc.comparison_func),
        .BorderColor = { desc.border_color[0], desc.border_color[1], desc.border_color[2], desc.border_color[3] },
        .MinLOD = desc.min_lod,
        .MaxLOD = desc.max_lod
    };

    D3D12_CPU_DESCRIPTOR_HANDLE handle = get_cpu_descriptor_handle(index);
    m_Device->CreateSampler(&sampler_desc, handle);

    return index;
}

void* DX12DescriptorHeap::get_cpu_handle(u32 index) {
    static D3D12_CPU_DESCRIPTOR_HANDLE handle;
    handle = get_cpu_descriptor_handle(index);
    return &handle;
}

void* DX12DescriptorHeap::get_gpu_handle(u32 index) {
    static D3D12_GPU_DESCRIPTOR_HANDLE handle;
    handle = get_gpu_descriptor_handle(index);
    return &handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::get_cpu_descriptor_handle(u32 index) const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_CPUStart;
    handle.ptr += index * m_DescriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::get_gpu_descriptor_handle(u32 index) const {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = m_GPUStart;
    handle.ptr += index * m_DescriptorSize;
    return handle;
}

} // namespace sf::render::dx12
