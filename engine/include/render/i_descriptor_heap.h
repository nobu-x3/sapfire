#pragma once

#include "core/core.h"
#include "resource_types.h"

namespace sf::render {

// Descriptor Heap Interface

class IDescriptorHeap {
public:
    virtual ~IDescriptorHeap() = default;

    // Allocate descriptors (returns descriptor index for bindless access)
    virtual u32 allocate_srv(Buffer& buffer) = 0;
    virtual u32 allocate_srv(Texture& texture, const ShaderResourceViewDesc* desc = nullptr) = 0;

    virtual u32 allocate_uav(Buffer& buffer) = 0;
    virtual u32 allocate_uav(Texture& texture, const UnorderedAccessViewDesc* desc = nullptr) = 0;

    virtual u32 allocate_cbv(Buffer& buffer) = 0;
    virtual u32 allocate_cbv(const ConstantBufferViewDesc& desc) = 0;

    virtual u32 allocate_rtv(Texture& texture, const RenderTargetViewDesc* desc = nullptr) = 0;
    virtual u32 allocate_dsv(Texture& texture, const DepthStencilViewDesc* desc = nullptr) = 0;

    virtual u32 allocate_sampler(const SamplerDesc& desc) = 0;

    // Query descriptor heap properties
    virtual u32 get_descriptor_count() const = 0;
    virtual u32 get_allocated_count() const = 0;

    // Backend-specific handles (for advanced usage)
    virtual void* get_native_heap() = 0;
    virtual void* get_cpu_handle(u32 index) = 0;
    virtual void* get_gpu_handle(u32 index) = 0;
};

} // namespace sf::render
