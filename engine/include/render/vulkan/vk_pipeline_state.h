#pragma once
#include <vulkan/vulkan.h>
#include "render/i_pipeline_state.h"
#include "render/resource_types.h"

namespace sf::render::vk {

    class VkPipelineState : public IPipelineState {
    public:
        VkPipelineState() = default;
        ~VkPipelineState() override;

        void create_graphics(VkDevice device, const GraphicsPipelineStateDesc& desc, VkPipelineLayout layout, VkRenderPass render_pass);
        void create_compute(VkDevice device, const ComputePipelineStateDesc& desc, VkPipelineLayout layout);

        bool is_compute() const override { return m_IsCompute; }
        bool is_graphics() const override { return !m_IsCompute; }
        void* get_native_pipeline() override { return reinterpret_cast<void*>(m_Pipeline); }

        VkPipeline get_vk_pipeline() const { return m_Pipeline; }

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
        bool m_IsCompute = false;
    };

} // namespace sf::render::vk
