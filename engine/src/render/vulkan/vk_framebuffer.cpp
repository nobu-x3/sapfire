#include "render/vulkan/vk_framebuffer.h"
#include "core/logger.h"
#include "engpch.h"
#include "render/vulkan/vk_graphics_device.h"
#include "render/vulkan/vk_render_pass.h"

namespace sf::render::vk {

    VulkanFramebuffer::VulkanFramebuffer(VulkanGraphicsDevice* device, const FramebufferDesc& desc) :
        m_Device(device), m_Framebuffer(VK_NULL_HANDLE), m_Width(desc.width), m_Height(desc.height) {
        stl::vector<VkImageView> attachments(mem::MemTag::Render);
        for (Texture* color_tex : desc.color_attachments) {
            if (color_tex && color_tex->image_view) {
                VkImageView view = reinterpret_cast<VkImageView>(color_tex->image_view);
                attachments.push_back(view);
            }
        }
        if (desc.depth_attachment && desc.depth_attachment->image_view) {
            VkImageView view = reinterpret_cast<VkImageView>(desc.depth_attachment->image_view);
            attachments.push_back(view);
        }
        auto* vk_render_pass = static_cast<VulkanRenderPass*>(desc.render_pass);
        VkRenderPass render_pass_handle = vk_render_pass->get_vk_render_pass();
        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass_handle;
        framebuffer_info.attachmentCount = static_cast<u32>(attachments.size());
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = m_Width;
        framebuffer_info.height = m_Height;
        framebuffer_info.layers = 1;
        VkResult result = vkCreateFramebuffer(m_Device->get_vk_device(), &framebuffer_info, nullptr, &m_Framebuffer);
        if (result != VK_SUCCESS) {
            CORE_ERROR("Failed to create Vulkan framebuffer: {}", static_cast<i32>(result));
        }
        CORE_TRACE("Created Vulkan framebuffer: {} ({}x{})", desc.name, m_Width, m_Height);
    }

    VulkanFramebuffer::~VulkanFramebuffer() {
        if (m_Framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(m_Device->get_vk_device(), m_Framebuffer, nullptr);
            CORE_TRACE("Destroyed Vulkan framebuffer");
        }
    }

} // namespace sf::render::vk
