#include "render/vulkan/vk_render_pass.h"
#include "core/logger.h"
#include "engpch.h"
#include "render/vulkan/vk_graphics_device.h"
#include "render/vulkan/vk_type_conversions.h"

namespace sf::render::vk {

    VulkanRenderPass::VulkanRenderPass(VulkanGraphicsDevice* device, const RenderPassDesc& desc) :
        m_Device(device), m_Desc(desc), m_RenderPass(VK_NULL_HANDLE) {
        stl::vector<VkAttachmentDescription> attachments(mem::MemTag::Render);
        stl::vector<VkAttachmentReference> color_attachment_refs(mem::MemTag::Render);
        VkAttachmentReference depth_attachment_ref{};
        bool has_depth = m_Desc.depth_attachment.has_value();
        u32 attachment_index = 0;
        for (const auto& color_desc : m_Desc.color_attachments) {
            VkAttachmentDescription attachment{};
            attachment.format = to_vk_format(color_desc.format);
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = to_vk_attachment_load_op(color_desc.load_op);
            attachment.storeOp = to_vk_attachment_store_op(color_desc.store_op);
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = to_vk_image_layout(color_desc.initial_layout);
            attachment.finalLayout = to_vk_image_layout(color_desc.final_layout);
            attachments.push_back(attachment);
            VkAttachmentReference ref{};
            ref.attachment = attachment_index;
            ref.layout = attachment.finalLayout;
            color_attachment_refs.push_back(ref);
            attachment_index++;
        }
        if (has_depth) {
            const auto& depth_desc = m_Desc.depth_attachment.value();
            VkAttachmentDescription attachment{};
            attachment.format = to_vk_format(depth_desc.format);
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = to_vk_attachment_load_op(depth_desc.load_op);
            attachment.storeOp = to_vk_attachment_store_op(depth_desc.store_op);
            attachment.stencilLoadOp = to_vk_attachment_load_op(depth_desc.stencil_load_op);
            attachment.stencilStoreOp = to_vk_attachment_store_op(depth_desc.stencil_store_op);
            attachment.initialLayout = to_vk_image_layout(depth_desc.initial_layout);
            attachment.finalLayout = to_vk_image_layout(depth_desc.final_layout);
            attachments.push_back(attachment);
            depth_attachment_ref.attachment = attachment_index;
            depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<u32>(color_attachment_refs.size());
        subpass.pColorAttachments = color_attachment_refs.empty() ? nullptr : color_attachment_refs.data();
        subpass.pDepthStencilAttachment = has_depth ? &depth_attachment_ref : nullptr;
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = static_cast<u32>(attachments.size());
        render_pass_info.pAttachments = attachments.data();
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies = &dependency;
        VkResult result = vkCreateRenderPass(m_Device->get_vk_device(), &render_pass_info, nullptr, &m_RenderPass);
        if (result != VK_SUCCESS) {
            CORE_ERROR("Failed to create Vulkan render pass: {}", static_cast<i32>(result));
        }
        CORE_TRACE("Created Vulkan render pass: {}", m_Desc.name);
    }

    VulkanRenderPass::~VulkanRenderPass() {
        if (m_RenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(m_Device->get_vk_device(), m_RenderPass, nullptr);
            CORE_TRACE("Destroyed Vulkan render pass: {}", m_Desc.name);
        }
    }

} // namespace sf::render::vk
