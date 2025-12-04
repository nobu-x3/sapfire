#pragma once

#include "core/core.h"
#include "i_descriptor_pool.h"
#include "i_descriptor_set.h"
#include "resource_types.h"

namespace sf::render {

    class ISampler;

    // Bindless Resource Registry
    // Manages bindless resource indices and descriptor updates
    // User-owned utility for managing bindless rendering resources

    class BindlessResourceRegistry {
    public:
        BindlessResourceRegistry(IDescriptorPool* pool, u32 max_textures = 100000, u32 max_buffers = 100000);
        ~BindlessResourceRegistry() = default;

        // Register resources and get their bindless indices
        u32 register_texture(Texture& texture, ISampler* sampler = nullptr);
        u32 register_buffer(Buffer& buffer);
        u32 register_constant_buffer(Buffer& buffer) { return register_buffer(buffer); }

        // Unregister resources (frees their indices for reuse)
        void unregister_texture(u32 index);
        void unregister_buffer(u32 index);

        // Get the descriptor set for binding
        IDescriptorSet* get_descriptor_set() const { return m_DescriptorSet.get(); }

        // Statistics
        u32 get_texture_count() const { return m_NextTextureIndex; }
        u32 get_buffer_count() const { return m_NextBufferIndex; }

    private:
        stl::unique_ptr<IDescriptorSet> m_DescriptorSet;
        u32 m_NextTextureIndex = 0;
        u32 m_NextBufferIndex = 0;
        u32 m_MaxTextures = 0;
        u32 m_MaxBuffers = 0;

        // Free lists for reusing indices
        stl::vector<u32> m_FreeTextureIndices{mem::MemTag::Render};
        stl::vector<u32> m_FreeBufferIndices{mem::MemTag::Render};
    };

} // namespace sf::render
