#pragma once
#include <vulkan/vulkan.h>
#include "render/i_pipeline_state.h"
#include "render/resource_types.h"

namespace sf::render::vk {

    class VulkanPipelineState final : public IPipelineState {
    public:
        VulkanPipelineState() = default;
        ~VulkanPipelineState() override;

        stl::result<> create_graphics(VkDevice device, const GraphicsPipelineDesc& desc, VkPipelineLayout layout,
                                      VkRenderPass render_pass);
        stl::result<> create_compute(VkDevice device, const ComputePipelineDesc& desc, VkPipelineLayout layout);

        bool is_compute() const override { return m_IsCompute; }
        bool is_graphics() const override { return !m_IsCompute; }
        void* get_native_pipeline() override { return reinterpret_cast<void*>(m_Pipeline); }
        IPipelineLayout* get_layout() const override { return m_Layout; }

        VkPipeline get_vk_pipeline() const { return m_Pipeline; }

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
        IPipelineLayout* m_Layout = nullptr;
        bool m_IsCompute = false;
    };

} // namespace sf::render::vk
