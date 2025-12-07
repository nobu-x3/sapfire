#pragma once

#include <vulkan/vulkan.h>
#include "render/i_pipeline_layout.h"

namespace sf::render::vk {

    class VulkanGraphicsDevice;

    class VulkanPipelineLayout final : public IPipelineLayout {
    public:
        VulkanPipelineLayout(VulkanGraphicsDevice* device, const PipelineLayoutDesc& desc);
        ~VulkanPipelineLayout() override;

        // IPipelineLayout interface
        const PipelineLayoutDesc& get_desc() const override { return m_Desc; }
        void* get_native_layout() override { return reinterpret_cast<void*>(m_PipelineLayout); }

        // Vulkan-specific accessors
        VkPipelineLayout get_vk_pipeline_layout() const { return m_PipelineLayout; }
        const stl::vector<VkDescriptorSetLayout>& get_vk_descriptor_set_layouts() const { return m_DescriptorSetLayouts; }

    private:
        VulkanGraphicsDevice* m_Device;
        PipelineLayoutDesc m_Desc;
        VkPipelineLayout m_PipelineLayout;
        stl::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts{mem::MemTag::Render};
    };

} // namespace sf::render::vk
