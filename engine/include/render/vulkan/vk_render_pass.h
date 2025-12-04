#pragma once

#include <vulkan/vulkan.h>
#include "render/i_render_pass.h"

namespace sf::render::vk {

    class VulkanGraphicsDevice;

    class VulkanRenderPass final : public IRenderPass {
    public:
        VulkanRenderPass(VulkanGraphicsDevice* device, const RenderPassDesc& desc);
        ~VulkanRenderPass() override;

        // IRenderPass interface
        const RenderPassDesc& get_desc() const override { return m_Desc; }
        void* get_native_render_pass() override { return reinterpret_cast<void*>(m_RenderPass); }

        // Vulkan-specific accessors
        VkRenderPass get_vk_render_pass() const { return m_RenderPass; }

    private:
        VulkanGraphicsDevice* m_Device;
        RenderPassDesc m_Desc;
        VkRenderPass m_RenderPass;
    };

} // namespace sf::render::vk
