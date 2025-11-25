#pragma once

#include "core/core.h"
#include "resource_types.h"

namespace sf::render {

    // Memory Allocator Interface

    class IMemoryAllocator {
    public:
        virtual ~IMemoryAllocator() = default;

        // Buffer allocation
        virtual void allocate_buffer(Buffer& buffer, const BufferCreationDesc& desc) = 0;

        // Texture allocation
        virtual void allocate_texture(Texture& texture, const TextureCreationDesc& desc) = 0;

        // Free resources
        virtual void free_buffer(Buffer& buffer) = 0;
        virtual void free_texture(Texture& texture) = 0;

        // Memory stats
        virtual void get_stats(void* stats_out) = 0; // Backend-specific stats structure

        // Backend-specific handle
        virtual void* get_native_allocator() = 0;
    };

} // namespace sf::render
