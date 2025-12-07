#include "engpch.h"

#include "render/bindless_resource_registry.h"
#include "core/logger.h"
#include "render/i_pipeline_layout.h"
#include "render/i_sampler.h"
#include <cassert>

namespace sf::render {

    stl::vector<DescriptorSetLayout> BindlessResourceRegistry::default_descriptor_set_layout(u32 max_textures, u32 max_buffers) {
        stl::vector<DescriptorSetLayout> layouts{mem::MemTag::Render};
        layouts.resize(2);  // Set 0: Textures and Samplers, Set 1: Buffers
        // Set 0: Textures and Samplers
        DescriptorSetLayout& set0 = layouts[0];
        set0.bindings.reserve(2);
        // Binding 0: Texture array (sampled images) - fixed size
        set0.bindings.push_back({
            .binding = 0,
            .type = DescriptorType::SampledImage,
            .count = max_textures,
            .stages = static_cast<ShaderStage>(static_cast<u32>(ShaderStage::Vertex) | static_cast<u32>(ShaderStage::Pixel)),
            .variable_count = false,
        });
        // Binding 1: Sampler - single sampler for now (must be last in this set)
        set0.bindings.push_back({
            .binding = 1,
            .type = DescriptorType::Sampler,
            .count = 1,
            .stages = static_cast<ShaderStage>(static_cast<u32>(ShaderStage::Vertex) | static_cast<u32>(ShaderStage::Pixel)),
            .variable_count = false,
        });
        // Set 1: Buffers (can have variable count since it's the only/last binding in this set)
        DescriptorSetLayout& set1 = layouts[1];
        set1.bindings.reserve(1);
        set1.bindings.push_back({
            .binding = 0,
            .type = DescriptorType::StorageBuffer,
            .count = max_buffers,
            .stages = static_cast<ShaderStage>(static_cast<u32>(ShaderStage::Vertex) | static_cast<u32>(ShaderStage::Pixel)),
            .variable_count = true,  // Can be variable since it's the only binding in Set 1
        });
        return layouts;
    }

    BindlessResourceRegistry::BindlessResourceRegistry(IDescriptorPool* pool, stl::span<const DescriptorSetLayout> layouts) {
        // Validate we have at least one layout
        if (layouts.empty()) {
            CORE_ERROR("BindlessResourceRegistry requires at least one descriptor set layout");
            return;
        }
        // Allocate descriptor sets and scan for binding information
        m_DescriptorSets.reserve(layouts.size());
        for (u32 setIndex = 0; setIndex < layouts.size(); ++setIndex) {
            const DescriptorSetLayout& layout = layouts[setIndex];
            // Scan bindings in this set
            for (const auto& binding : layout.bindings) {
                if (binding.type == DescriptorType::SampledImage) {
                    m_TextureSetIndex = setIndex;
                    m_TextureBinding = binding.binding;
                    m_MaxTextures = binding.count;
                } else if (binding.type == DescriptorType::StorageBuffer) {
                    m_BufferSetIndex = setIndex;
                    m_BufferBinding = binding.binding;
                    m_MaxBuffers = binding.count;
                } else if (binding.type == DescriptorType::Sampler) {
                    m_SamplerSetIndex = setIndex;
                    m_SamplerBinding = binding.binding;
                }
            }
            // Allocate descriptor set for this layout
            auto result = pool->allocate_set(layout);
            if (!result) {
                CORE_ERROR("Failed to allocate descriptor set {} for bindless registry", setIndex);
                return;
            }
            m_DescriptorSets.push_back(std::move(*result));
        }
        // Validate we found the required bindings
        if (m_TextureSetIndex == UINT32_MAX) {
            CORE_WARN("BindlessResourceRegistry: No SampledImage binding found");
        }
        if (m_BufferSetIndex == UINT32_MAX) {
            CORE_WARN("BindlessResourceRegistry: No StorageBuffer binding found");
        }
        m_FreeTextureIndices.reserve(1024);
        m_FreeBufferIndices.reserve(1024);
        CORE_INFO("Created bindless resource registry ({} sets, max textures: {}, max buffers: {})",
                  m_DescriptorSets.size(), m_MaxTextures, m_MaxBuffers);
    }

    IDescriptorSet* BindlessResourceRegistry::get_descriptor_set(u32 set_index) const {
        if (set_index >= m_DescriptorSets.size()) {
            CORE_ERROR("Invalid descriptor set index: {} (max: {})", set_index, m_DescriptorSets.size() - 1);
            return nullptr;
        }
        return m_DescriptorSets[set_index].get();
    }

    u32 BindlessResourceRegistry::register_texture(Texture& texture, ISampler* sampler) {
        assert(m_TextureSetIndex != UINT32_MAX && "No texture set index configured");
        assert(m_TextureBinding != UINT32_MAX && "No texture binding configured");
        u32 index = 0;
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
        stl::array<Texture*, 1> textures = {&texture};
        m_DescriptorSets[m_TextureSetIndex]->write_texture_array(m_TextureBinding, textures, sampler, index);
        return index;
    }

    u32 BindlessResourceRegistry::register_buffer(Buffer& buffer) {
        assert(m_BufferSetIndex != UINT32_MAX && "No buffer set index configured");
        assert(m_BufferBinding != UINT32_MAX && "No buffer binding configured");
        u32 index = 0;
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
        stl::array<Buffer*, 1> buffers = {&buffer};
        m_DescriptorSets[m_BufferSetIndex]->write_buffer_array(m_BufferBinding, buffers, index);
        return index;
    }

    void BindlessResourceRegistry::set_default_sampler(ISampler* sampler) {
        assert(m_SamplerSetIndex != UINT32_MAX && "No sampler set index configured");
        assert(m_SamplerBinding != UINT32_MAX && "No sampler binding configured");
        assert(sampler != nullptr && "Sampler cannot be null");
        m_DescriptorSets[m_SamplerSetIndex]->write_sampler(m_SamplerBinding, sampler);
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
