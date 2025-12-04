#include "render/bindless_resource_registry.h"
#include "core/logger.h"
#include "engpch.h"
#include "render/i_pipeline_layout.h"
#include "render/i_sampler.h"

namespace sf::render {

    BindlessResourceRegistry::BindlessResourceRegistry(IDescriptorPool* pool, u32 max_textures, u32 max_buffers) :
        m_MaxTextures(max_textures), m_MaxBuffers(max_buffers) {
        // Create descriptor set layout for bindless resources
        // Binding 0: Texture array (sampled images)
        // Binding 1: Buffer array (storage buffers)
        DescriptorSetLayout layout;
        layout.bindings.reserve(2);
        layout.bindings.push_back({
            .binding = 0,
            .type = DescriptorType::SampledImage,
            .count = max_textures,
            .stages = ShaderStage::Pixel,
            .variable_count = true,
        });
        layout.bindings.push_back({
            .binding = 1,
            .type = DescriptorType::StorageBuffer,
            .count = max_buffers,
            .stages = static_cast<ShaderStage>(static_cast<u32>(ShaderStage::Vertex) | static_cast<u32>(ShaderStage::Pixel)),
            .variable_count = true,
        });
        auto result = pool->allocate_set(layout);
        if (!result) {
            CORE_ERROR("Failed to allocate descriptor set for bindless registry");
            return;
        }
        m_DescriptorSet = std::move(*result);
        m_FreeTextureIndices.reserve(1024);
        m_FreeBufferIndices.reserve(1024);
        CORE_INFO("Created bindless resource registry (max textures: {}, max buffers: {})", max_textures, max_buffers);
    }

    u32 BindlessResourceRegistry::register_texture(Texture& texture, ISampler* sampler) {
        u32 index = 0;
        // Reuse a free index if available
        if (!m_FreeTextureIndices.empty()) {
            index = m_FreeTextureIndices.back();
            m_FreeTextureIndices.pop_back();
        } else {
            if (m_NextTextureIndex >= m_MaxTextures) {
                CORE_ERROR("Bindless texture registry is full (max: {})", m_MaxTextures);
                return UINT32_MAX;
            }
            index = m_NextTextureIndex++;
        }
        // Write descriptor at this index
        stl::array<Texture*, 1> textures = {&texture};
        m_DescriptorSet->write_texture_array(0, textures, sampler, index);
        return index;
    }

    u32 BindlessResourceRegistry::register_buffer(Buffer& buffer) {
        u32 index = 0;
        // Reuse a free index if available
        if (!m_FreeBufferIndices.empty()) {
            index = m_FreeBufferIndices.back();
            m_FreeBufferIndices.pop_back();
        } else {
            if (m_NextBufferIndex >= m_MaxBuffers) {
                CORE_ERROR("Bindless buffer registry is full (max: {})", m_MaxBuffers);
                return UINT32_MAX;
            }
            index = m_NextBufferIndex++;
        }
        // Write descriptor at this index
        stl::array<Buffer*, 1> buffers = {&buffer};
        m_DescriptorSet->write_buffer_array(1, buffers, index);
        return index;
    }

    void BindlessResourceRegistry::unregister_texture(u32 index) {
        if (index < m_NextTextureIndex) {
            m_FreeTextureIndices.push_back(index);
        }
    }

    void BindlessResourceRegistry::unregister_buffer(u32 index) {
        if (index < m_NextBufferIndex) {
            m_FreeBufferIndices.push_back(index);
        }
    }

} // namespace sf::render
