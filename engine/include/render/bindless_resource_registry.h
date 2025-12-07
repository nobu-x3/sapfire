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

    struct DescriptorSetLayout;

    class BindlessResourceRegistry {
    public:
        BindlessResourceRegistry(IDescriptorPool* pool, stl::span<const DescriptorSetLayout> layouts);
        ~BindlessResourceRegistry() = default;

        // Create a default descriptor set layout for bindless rendering
        // Returns a vector containing one layout with texture and buffer bindings
        static stl::vector<DescriptorSetLayout> default_descriptor_set_layout(u32 max_textures = 100000, u32 max_buffers = 100000);

        // Register resources and get their bindless indices
        u32 register_texture(Texture& texture, ISampler* sampler = nullptr);
        u32 register_buffer(Buffer& buffer);
        void set_default_sampler(ISampler* sampler);

        // Unregister resources (frees their indices for reuse)
        void unregister_texture(u32 index);
        void unregister_buffer(u32 index);

        // Get descriptor sets for binding
        // For single-set layouts, use index 0. For multi-set layouts, use appropriate indices.
        IDescriptorSet* get_descriptor_set(u32 set_index = 0) const;
        u32 get_descriptor_set_count() const { return static_cast<u32>(m_DescriptorSets.size()); }

        // Statistics
        u32 get_texture_count() const { return m_NextTextureIndex; }
        u32 get_buffer_count() const { return m_NextBufferIndex; }

    private:
        stl::vector<stl::unique_ptr<IDescriptorSet>> m_DescriptorSets{mem::MemTag::Render};
        u32 m_TextureSetIndex = UINT32_MAX;
        u32 m_TextureBinding = UINT32_MAX;
        u32 m_BufferSetIndex = UINT32_MAX;
        u32 m_BufferBinding = UINT32_MAX;
        u32 m_SamplerSetIndex = UINT32_MAX;
        u32 m_SamplerBinding = UINT32_MAX;
        u32 m_NextTextureIndex = 0;
        u32 m_NextBufferIndex = 0;
        u32 m_MaxTextures = 0;
        u32 m_MaxBuffers = 0;

        // Free lists for reusing indices
        stl::vector<u32> m_FreeTextureIndices{mem::MemTag::Render};
        stl::vector<u32> m_FreeBufferIndices{mem::MemTag::Render};
    };

} // namespace sf::render
