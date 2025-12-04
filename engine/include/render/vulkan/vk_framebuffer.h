#pragma once

#include <vulkan/vulkan.h>
#include "render/i_framebuffer.h"

namespace sf::render::vk {

    class VulkanGraphicsDevice;

    class VulkanFramebuffer final : public IFramebuffer {
    public:
        VulkanFramebuffer(VulkanGraphicsDevice* device, const FramebufferDesc& desc);
        ~VulkanFramebuffer() override;

        // IFramebuffer interface
        u32 get_width() const override { return m_Width; }
        u32 get_height() const override { return m_Height; }
        void* get_native_framebuffer() override { return reinterpret_cast<void*>(m_Framebuffer); }

        // Vulkan-specific accessors
        VkFramebuffer get_vk_framebuffer() const { return m_Framebuffer; }

    private:
        VulkanGraphicsDevice* m_Device;
        VkFramebuffer m_Framebuffer;
        u32 m_Width;
        u32 m_Height;
    };

} // namespace sf::render::vk
