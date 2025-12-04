#pragma once

#include <vulkan/vulkan.h>
#include "render/i_descriptor_set.h"
#include "render/i_pipeline_layout.h"

namespace sf::render::vk {

    class VulkanGraphicsDevice;
    class VulkanDescriptorPool;

    class VulkanDescriptorSet final : public IDescriptorSet {
    public:
        VulkanDescriptorSet(VulkanGraphicsDevice* device, VulkanDescriptorPool* pool, const DescriptorSetLayout& layout);
        ~VulkanDescriptorSet() override;

        // IDescriptorSet interface
        void write_buffer(u32 binding, Buffer& buffer, u64 offset = 0, u64 range = UINT64_MAX) override;
        void write_texture(u32 binding, Texture& texture, ISampler* sampler = nullptr) override;
        void write_sampler(u32 binding, ISampler* sampler) override;
        void write_buffer_array(u32 binding, stl::span<Buffer*> buffers, u32 array_element = 0) override;
        void write_texture_array(u32 binding, stl::span<Texture*> textures, ISampler* sampler = nullptr, u32 array_element = 0) override;

        void* get_native_descriptor_set() override { return reinterpret_cast<void*>(m_DescriptorSet); }

        // Vulkan-specific accessors
        VkDescriptorSet get_vk_descriptor_set() const { return m_DescriptorSet; }

    private:
        VulkanGraphicsDevice* m_Device;
        VkDescriptorPool m_Pool;
        VkDescriptorSet m_DescriptorSet;
    };

} // namespace sf::render::vk
