#pragma once

#include "core/core.h"

namespace sf::render {

    class IDescriptorSet;
    struct DescriptorSetLayout;

    // Descriptor Pool Description
    struct DescriptorPoolDesc {
        u32 max_sets = 0;
        u32 max_uniform_buffers = 0;
        u32 max_storage_buffers = 0;
        u32 max_sampled_images = 0;
        u32 max_storage_images = 0;
        u32 max_samplers = 0;
        const char* name = "Descriptor Pool";
    };

    // Descriptor Pool Interface
    // User-managed pool for allocating descriptor sets
    class IDescriptorPool {
    public:
        virtual ~IDescriptorPool() = default;

        // Allocate a descriptor set from this pool
        virtual stl::result<stl::unique_ptr<IDescriptorSet>> allocate_set(const DescriptorSetLayout& layout) = 0;

        // Reset the pool (frees all allocated sets)
        virtual void reset() = 0;

        // Backend-specific handle
        virtual void* get_native_pool() = 0;
    };

} // namespace sf::render
