#pragma once

#include <span>
#include "core/core.h"
#include "resource_types.h"

namespace sf::render {

    class ISampler;

    // Descriptor Set Interface
    // Replaces the old IDescriptorHeap with explicit descriptor sets

    class IDescriptorSet {
    public:
        virtual ~IDescriptorSet() = default;

        // Write single descriptors
        virtual void write_buffer(u32 binding, Buffer& buffer, u64 offset = 0, u64 range = UINT64_MAX) = 0;
        virtual void write_texture(u32 binding, Texture& texture, ISampler* sampler = nullptr) = 0;
        virtual void write_sampler(u32 binding, ISampler* sampler) = 0;

        // Write descriptor arrays (for bindless rendering)
        virtual void write_buffer_array(u32 binding, stl::span<Buffer*> buffers, u32 array_element = 0) = 0;
        virtual void write_texture_array(u32 binding, stl::span<Texture*> textures, ISampler* sampler = nullptr, u32 array_element = 0) = 0;

        // Backend-specific handle
        virtual void* get_native_descriptor_set() = 0;
    };

} // namespace sf::render
