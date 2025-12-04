#include "engpch.h"

#include <vulkan/vulkan_core.h>
#include "core/logger.h"
#include "render/vulkan/vk_compat.h"
#include "render/vulkan/vk_context.h"
#include "render/vulkan/vk_descriptor_set.h"
#include "render/vulkan/vk_framebuffer.h"
#include "render/vulkan/vk_graphics_device.h"
#include "render/vulkan/vk_pipeline_layout.h"
#include "render/vulkan/vk_pipeline_state.h"
#include "render/vulkan/vk_render_pass.h"
#include "render/vulkan/vk_type_conversions.h"

namespace sf::render::vk {
    // Base VulkanContext

    VulkanContext::VulkanContext(VulkanGraphicsDevice* device, u32 family_index, VkCommandPoolCreateFlags pool_ci_flags) :
        m_Device(nullptr) {
        init(device, family_index, pool_ci_flags);
    }

    void VulkanContext::destroy_resources() {
        if (m_Device == VK_NULL_HANDLE)
            return;
        m_ImageBarriers.clear();
        m_BufferBarriers.clear();
        if (m_Device != VK_NULL_HANDLE && m_CommandBuffer != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &m_CommandBuffer);
            m_CommandBuffer = VK_NULL_HANDLE;
        }
        if (m_Device != VK_NULL_HANDLE && m_CommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }
        m_Device = VK_NULL_HANDLE;
    }

    stl::result<> VulkanContext::init(VulkanGraphicsDevice* device, u32 family_index, VkCommandPoolCreateFlags pool_ci_flags) {
        m_Device = device->get_vk_device();
        VkCommandPoolCreateInfo command_pool_ci{};
        command_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_ci.pNext = nullptr;
        command_pool_ci.flags = pool_ci_flags;
        command_pool_ci.queueFamilyIndex = family_index;
        VK_RETURN_ON_ERROR(vkCreateCommandPool(m_Device, &command_pool_ci, nullptr, &m_CommandPool), "Failed to create command pool.");
        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.commandBufferCount = 1;
        alloc_info.commandPool = m_CommandPool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        VK_RETURN_ON_ERROR(vkAllocateCommandBuffers(m_Device, &alloc_info, &m_CommandBuffer), "Failed to allocate command buffer.");
        return stl::success;
    }

    stl::result<> VulkanContext::reset() {
        m_ImageBarriers.clear();
        m_BufferBarriers.clear();
        if (m_CommandBuffer != VK_NULL_HANDLE) {
            VK_RETURN_ON_ERROR(vkResetCommandBuffer(m_CommandBuffer, 0), "Failed to reset command buffer in VulkanContext.");
            VkCommandBufferBeginInfo begin_info{};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            begin_info.pInheritanceInfo = nullptr;
            VK_RETURN_ON_ERROR(vkBeginCommandBuffer(m_CommandBuffer, &begin_info), "Failed to begin command buffer in VulkanContext.");
        }
        return stl::success;
    }

    stl::result<> VulkanContext::close() {
        if (m_CommandBuffer != VK_NULL_HANDLE) {
            VK_RETURN_ON_ERROR(vkEndCommandBuffer(m_CommandBuffer), "Failed to end command buffer in VulkanContext.");
        }
        return stl::success;
    }

    VulkanGraphicsContext::VulkanGraphicsContext(VulkanGraphicsDevice* device) :
        VulkanContext(device, device->get_graphics_queue_family_index(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
        m_DevicePtr(device) {}

    stl::result<> VulkanGraphicsContext::reset() {
        if (m_CurrentRenderPass != VK_NULL_HANDLE) {
            vkCmdEndRenderPass(m_CommandBuffer);
            m_CurrentRenderPass = VK_NULL_HANDLE;
            m_CurrentFramebuffer = VK_NULL_HANDLE;
        }
        return VulkanContext::reset();
    }

    stl::result<> VulkanGraphicsContext::close() {
        // End any active render pass before closing command buffer
        if (m_CurrentRenderPass != VK_NULL_HANDLE) {
            vkCmdEndRenderPass(m_CommandBuffer);
            m_CurrentRenderPass = VK_NULL_HANDLE;
            m_CurrentFramebuffer = VK_NULL_HANDLE;
        }
        return VulkanContext::close();
    }

    void VulkanGraphicsContext::begin_render_pass(IRenderPass* render_pass, IFramebuffer* framebuffer) {
        auto* vk_render_pass = static_cast<VulkanRenderPass*>(render_pass);
        auto* vk_framebuffer = static_cast<VulkanFramebuffer*>(framebuffer);
        m_CurrentRenderPass = vk_render_pass->get_vk_render_pass();
        m_CurrentFramebuffer = vk_framebuffer->get_vk_framebuffer();
        VkRenderPassBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begin_info.renderPass = m_CurrentRenderPass;
        begin_info.framebuffer = m_CurrentFramebuffer;
        begin_info.renderArea.offset = {0, 0};
        begin_info.renderArea.extent = {vk_framebuffer->get_width(), vk_framebuffer->get_height()};
        // Set clear values from render pass description
        const auto& desc = vk_render_pass->get_desc();
        stl::vector<VkClearValue> clear_values(mem::MemTag::Temp);
        for (const auto& attachment : desc.color_attachments) {
            if (attachment.load_op == LoadOp::Clear) {
                VkClearValue clear{};
                clear.color = {{0.0f, 0.0f, 0.0f, 1.0f}}; // Default black
                clear_values.push_back(clear);
            } else {
                clear_values.push_back({});
            }
        }
        if (desc.depth_attachment.has_value() && desc.depth_attachment->load_op == LoadOp::Clear) {
            VkClearValue clear{};
            clear.depthStencil = {1.0f, 0};
            clear_values.push_back(clear);
        }
        begin_info.clearValueCount = static_cast<u32>(clear_values.size());
        begin_info.pClearValues = clear_values.data();
        vkCmdBeginRenderPass(m_CommandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanGraphicsContext::end_render_pass() {
        if (m_CurrentRenderPass == VK_NULL_HANDLE) {
            CORE_WARN("VulkanGraphicsContext::end_render_pass - no active render pass");
            return;
        }
        vkCmdEndRenderPass(m_CommandBuffer);
        m_CurrentRenderPass = VK_NULL_HANDLE;
        m_CurrentFramebuffer = VK_NULL_HANDLE;
    }

    void VulkanGraphicsContext::clear_render_target(u32 attachment_index, stl::span<f32, 4> clear_color) {
        if (m_CurrentRenderPass == VK_NULL_HANDLE) {
            CORE_WARN("VulkanGraphicsContext::clear_render_target - no active render pass");
            return;
        }
        VkClearAttachment attachment{};
        attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        attachment.colorAttachment = attachment_index;
        attachment.clearValue.color.float32[0] = clear_color[0];
        attachment.clearValue.color.float32[1] = clear_color[1];
        attachment.clearValue.color.float32[2] = clear_color[2];
        attachment.clearValue.color.float32[3] = clear_color[3];
        VkClearRect clear_rect{};
        clear_rect.rect.offset = {0, 0};
        clear_rect.rect.extent = {static_cast<u32>(m_Viewport.width), static_cast<u32>(m_Viewport.height)};
        clear_rect.baseArrayLayer = 0;
        clear_rect.layerCount = 1;
        vkCmdClearAttachments(m_CommandBuffer, 1, &attachment, 1, &clear_rect);
    }

    void VulkanGraphicsContext::clear_depth_stencil(f32 depth, u8 stencil) {
        if (m_CurrentRenderPass == VK_NULL_HANDLE) {
            CORE_WARN("VulkanGraphicsContext::clear_depth_stencil - no active render pass");
            return;
        }
        VkClearAttachment attachment{};
        attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if(stencil != 0) {
            attachment.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        attachment.clearValue.depthStencil.depth = depth;
        attachment.clearValue.depthStencil.stencil = stencil;
        VkClearRect clear_rect{};
        clear_rect.rect.offset = {0, 0};
        clear_rect.rect.extent = {static_cast<u32>(m_Viewport.width), static_cast<u32>(m_Viewport.height)};
        clear_rect.baseArrayLayer = 0;
        clear_rect.layerCount = 1;
        vkCmdClearAttachments(m_CommandBuffer, 1, &attachment, 1, &clear_rect);
    }

    void VulkanGraphicsContext::bind_pipeline(IPipelineState* pipeline) {
        if (!pipeline)
            return;
        auto* vk_pipeline = static_cast<VulkanPipelineState*>(pipeline);
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->get_vk_pipeline());
        // Store pipeline layout for descriptor set binding
        auto* layout_ptr = vk_pipeline->get_layout();
        if (layout_ptr) {
            auto* vk_layout = static_cast<VulkanPipelineLayout*>(layout_ptr);
            m_CurrentPipelineLayout = vk_layout->get_vk_pipeline_layout();
        }
    }

    void VulkanGraphicsContext::bind_descriptor_set(u32 set_index, IDescriptorSet* descriptor_set) {
        if (!descriptor_set || m_CurrentPipelineLayout == VK_NULL_HANDLE) {
            CORE_WARN("VulkanGraphicsContext::bind_descriptor_set - invalid descriptor set or no pipeline bound");
            return;
        }
        auto* vk_desc_set = static_cast<VulkanDescriptorSet*>(descriptor_set);
        VkDescriptorSet set = vk_desc_set->get_vk_descriptor_set();
        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CurrentPipelineLayout, set_index, 1, &set, 0, nullptr);
    }

    void VulkanGraphicsContext::push_constants(const void* data, u32 size, u32 offset) {
        if (m_CurrentPipelineLayout == VK_NULL_HANDLE) {
            CORE_WARN("VulkanGraphicsContext::push_constants - no pipeline bound");
            return;
        }
        vkCmdPushConstants(m_CommandBuffer, m_CurrentPipelineLayout, VK_SHADER_STAGE_ALL, offset, size, data);
    }

    void VulkanGraphicsContext::set_scissor(const ScissorRect& scissor) {
        VkRect2D vk_scissor{};
        vk_scissor.offset.x = static_cast<i32>(scissor.left);
        vk_scissor.offset.y = static_cast<i32>(scissor.top);
        vk_scissor.extent.width = static_cast<u32>(scissor.right - scissor.left);
        vk_scissor.extent.height = static_cast<u32>(scissor.bottom - scissor.top);
        vkCmdSetScissor(m_CommandBuffer, 0, 1, &vk_scissor);
    }

    void VulkanGraphicsContext::set_viewport(const Viewport& viewport) {
        m_Viewport.x = viewport.x;
        m_Viewport.y = viewport.y;
        m_Viewport.width = viewport.width;
        m_Viewport.height = viewport.height;
        m_Viewport.minDepth = viewport.min_depth;
        m_Viewport.maxDepth = viewport.max_depth;
        vkCmdSetViewport(m_CommandBuffer, 0, 1, &m_Viewport);
    }

    void VulkanGraphicsContext::set_primitive_topology(PrimitiveTopology topology) {
        // Primitive topology is baked into pipeline state in Vulkan. No-op for compatibility.
        (void)topology;
    }

    void VulkanGraphicsContext::bind_vertex_buffer(u32 binding, Buffer& buffer, u64 offset) {
        VkBuffer vk_buffer = reinterpret_cast<VkBuffer>(buffer.resource);
        VkDeviceSize vk_offset = offset;
        vkCmdBindVertexBuffers(m_CommandBuffer, binding, 1, &vk_buffer, &vk_offset);
    }

    void VulkanGraphicsContext::bind_index_buffer(Buffer& buffer, Format format, u64 offset) {
        VkBuffer vk_buffer = reinterpret_cast<VkBuffer>(buffer.resource);
        VkIndexType index_type = (format == Format::R16_UINT) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
        vkCmdBindIndexBuffer(m_CommandBuffer, vk_buffer, offset, index_type);
    }

    void VulkanGraphicsContext::draw(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
        vkCmdDraw(m_CommandBuffer, vertex_count, instance_count, first_vertex, first_instance);
    }

    void VulkanGraphicsContext::draw_indexed(u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) {
        vkCmdDrawIndexed(m_CommandBuffer, index_count, instance_count, first_index, vertex_offset, first_instance);
    }

    void VulkanGraphicsContext::pipeline_barrier(const PipelineBarrier& barrier) {
        stl::vector<VkMemoryBarrier> memory_barriers(mem::MemTag::Temp);
        stl::vector<VkBufferMemoryBarrier> buffer_barriers(mem::MemTag::Temp);
        stl::vector<VkImageMemoryBarrier> image_barriers(mem::MemTag::Temp);
        for (const auto& mb : barrier.memory_barriers) {
            VkMemoryBarrier vk_barrier{};
            vk_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            vk_barrier.srcAccessMask = static_cast<VkAccessFlags>(mb.src_access);
            vk_barrier.dstAccessMask = static_cast<VkAccessFlags>(mb.dst_access);
            memory_barriers.push_back(vk_barrier);
        }
        for (const auto& bb : barrier.buffer_barriers) {
            VkBufferMemoryBarrier vk_barrier{};
            vk_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            vk_barrier.srcAccessMask = static_cast<VkAccessFlags>(bb.src_access);
            vk_barrier.dstAccessMask = static_cast<VkAccessFlags>(bb.dst_access);
            vk_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_barrier.buffer = reinterpret_cast<VkBuffer>(bb.buffer->resource);
            vk_barrier.offset = bb.offset;
            vk_barrier.size = bb.size;
            buffer_barriers.push_back(vk_barrier);
        }
        for (const auto& ib : barrier.image_barriers) {
            VkImageMemoryBarrier vk_barrier{};
            vk_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            vk_barrier.srcAccessMask = static_cast<VkAccessFlags>(ib.src_access);
            vk_barrier.dstAccessMask = static_cast<VkAccessFlags>(ib.dst_access);
            vk_barrier.oldLayout = to_vk_image_layout(ib.old_layout);
            vk_barrier.newLayout = to_vk_image_layout(ib.new_layout);
            vk_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_barrier.image = reinterpret_cast<VkImage>(ib.texture->resource);
            vk_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // TODO: Detect from format
            vk_barrier.subresourceRange.baseMipLevel = ib.base_mip_level;
            vk_barrier.subresourceRange.levelCount = ib.mip_level_count;
            vk_barrier.subresourceRange.baseArrayLayer = ib.base_array_layer;
            vk_barrier.subresourceRange.layerCount = ib.array_layer_count;
            image_barriers.push_back(vk_barrier);
        }
        VkPipelineStageFlags src_stage = static_cast<VkPipelineStageFlags>(barrier.src_stage);
        VkPipelineStageFlags dst_stage = static_cast<VkPipelineStageFlags>(barrier.dst_stage);
        vkCmdPipelineBarrier(m_CommandBuffer, src_stage, dst_stage, 0, static_cast<u32>(memory_barriers.size()), memory_barriers.data(),
                             static_cast<u32>(buffer_barriers.size()), buffer_barriers.data(), static_cast<u32>(image_barriers.size()),
                             image_barriers.data());
    }

    void VulkanGraphicsContext::transition_image_layout(Texture& texture, ResourceState old_state, ResourceState new_state) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = to_vk_image_layout(old_state);
        barrier.newLayout = to_vk_image_layout(new_state);
        barrier.srcAccessMask = to_vk_access_flags(old_state);
        barrier.dstAccessMask = to_vk_access_flags(new_state);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = reinterpret_cast<VkImage>(texture.resource);
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // TODO: Detect from format
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = texture.mip_levels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = texture.depth_or_array_size;
        vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);
    }

    void VulkanGraphicsContext::transition_buffer_state(Buffer& buffer, ResourceState old_state, ResourceState new_state) {
        VkBufferMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = to_vk_access_flags(old_state);
        barrier.dstAccessMask = to_vk_access_flags(new_state);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = reinterpret_cast<VkBuffer>(buffer.resource);
        barrier.offset = 0;
        barrier.size = buffer.size_in_bytes;
        vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1,
                             &barrier, 0, nullptr);
    }

    // VulkanComputeContext
    VulkanComputeContext::VulkanComputeContext(VulkanGraphicsDevice* device) :
        VulkanContext(device, device->get_compute_queue_family_index(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
        m_DevicePtr(device) {}

    stl::result<> VulkanComputeContext::reset() { return VulkanContext::reset(); }

    void VulkanComputeContext::dispatch(u32 thread_group_count_x, u32 thread_group_count_y, u32 thread_group_count_z) {
        vkCmdDispatch(m_CommandBuffer, thread_group_count_x, thread_group_count_y, thread_group_count_z);
    }

    void VulkanComputeContext::bind_pipeline(IPipelineState* pipeline) {
        if (!pipeline)
            return;
        auto* vk_pipeline = static_cast<VulkanPipelineState*>(pipeline);
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vk_pipeline->get_vk_pipeline());
        auto* layout_ptr = vk_pipeline->get_layout();
        if (layout_ptr) {
            auto* vk_layout = static_cast<VulkanPipelineLayout*>(layout_ptr);
            m_CurrentPipelineLayout = vk_layout->get_vk_pipeline_layout();
        }
    }

    void VulkanComputeContext::bind_descriptor_set(u32 set_index, IDescriptorSet* descriptor_set) {
        if (!descriptor_set || m_CurrentPipelineLayout == VK_NULL_HANDLE) {
            CORE_WARN("VulkanComputeContext::bind_descriptor_set - invalid descriptor set or no pipeline bound");
            return;
        }
        auto* vk_desc_set = static_cast<VulkanDescriptorSet*>(descriptor_set);
        VkDescriptorSet set = vk_desc_set->get_vk_descriptor_set();
        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_CurrentPipelineLayout, set_index, 1, &set, 0, nullptr);
    }

    void VulkanComputeContext::push_constants(const void* data, u32 size, u32 offset) {
        if (m_CurrentPipelineLayout == VK_NULL_HANDLE) {
            CORE_WARN("VulkanComputeContext::push_constants - no pipeline bound");
            return;
        }
        vkCmdPushConstants(m_CommandBuffer, m_CurrentPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, offset, size, data);
    }

    void VulkanComputeContext::pipeline_barrier(const PipelineBarrier& barrier) {
        stl::vector<VkMemoryBarrier> memory_barriers(mem::MemTag::Temp);
        stl::vector<VkBufferMemoryBarrier> buffer_barriers(mem::MemTag::Temp);
        stl::vector<VkImageMemoryBarrier> image_barriers(mem::MemTag::Temp);
        for (const auto& mb : barrier.memory_barriers) {
            VkMemoryBarrier vk_barrier{};
            vk_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            vk_barrier.srcAccessMask = static_cast<VkAccessFlags>(mb.src_access);
            vk_barrier.dstAccessMask = static_cast<VkAccessFlags>(mb.dst_access);
            memory_barriers.push_back(vk_barrier);
        }
        for (const auto& bb : barrier.buffer_barriers) {
            VkBufferMemoryBarrier vk_barrier{};
            vk_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            vk_barrier.srcAccessMask = static_cast<VkAccessFlags>(bb.src_access);
            vk_barrier.dstAccessMask = static_cast<VkAccessFlags>(bb.dst_access);
            vk_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_barrier.buffer = reinterpret_cast<VkBuffer>(bb.buffer->resource);
            vk_barrier.offset = bb.offset;
            vk_barrier.size = bb.size;
            buffer_barriers.push_back(vk_barrier);
        }
        for (const auto& ib : barrier.image_barriers) {
            VkImageMemoryBarrier vk_barrier{};
            vk_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            vk_barrier.srcAccessMask = static_cast<VkAccessFlags>(ib.src_access);
            vk_barrier.dstAccessMask = static_cast<VkAccessFlags>(ib.dst_access);
            vk_barrier.oldLayout = to_vk_image_layout(ib.old_layout);
            vk_barrier.newLayout = to_vk_image_layout(ib.new_layout);
            vk_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_barrier.image = reinterpret_cast<VkImage>(ib.texture->resource);
            vk_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // TODO: Detect from format
            vk_barrier.subresourceRange.baseMipLevel = ib.base_mip_level;
            vk_barrier.subresourceRange.levelCount = ib.mip_level_count;
            vk_barrier.subresourceRange.baseArrayLayer = ib.base_array_layer;
            vk_barrier.subresourceRange.layerCount = ib.array_layer_count;
            image_barriers.push_back(vk_barrier);
        }
        VkPipelineStageFlags src_stage = static_cast<VkPipelineStageFlags>(barrier.src_stage);
        VkPipelineStageFlags dst_stage = static_cast<VkPipelineStageFlags>(barrier.dst_stage);
        vkCmdPipelineBarrier(m_CommandBuffer, src_stage, dst_stage, 0, static_cast<u32>(memory_barriers.size()), memory_barriers.data(),
                             static_cast<u32>(buffer_barriers.size()), buffer_barriers.data(), static_cast<u32>(image_barriers.size()),
                             image_barriers.data());
    }

    VulkanCopyContext::VulkanCopyContext(VulkanGraphicsDevice* device) :
        VulkanContext(device, device->get_transfer_queue_family_index(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
        m_DevicePtr(device) {}

    stl::result<> VulkanCopyContext::reset() { return VulkanContext::reset(); }

    void VulkanCopyContext::copy_buffer(Buffer& dst, Buffer& src, u64 size, u64 dst_offset, u64 src_offset) {
        VkBufferCopy copy_region{};
        copy_region.srcOffset = src_offset;
        copy_region.dstOffset = dst_offset;
        copy_region.size = size;
        vkCmdCopyBuffer(m_CommandBuffer, reinterpret_cast<VkBuffer>(src.resource), reinterpret_cast<VkBuffer>(dst.resource), 1,
                        &copy_region);
    }

    void VulkanCopyContext::copy_texture(Texture& dst, Texture& src) {
        VkImageCopy copy_region{};
        copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.srcSubresource.mipLevel = 0;
        copy_region.srcSubresource.baseArrayLayer = 0;
        copy_region.srcSubresource.layerCount = 1;
        copy_region.srcOffset = {0, 0, 0};
        copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.dstSubresource.mipLevel = 0;
        copy_region.dstSubresource.baseArrayLayer = 0;
        copy_region.dstSubresource.layerCount = 1;
        copy_region.dstOffset = {0, 0, 0};
        copy_region.extent.width = std::min(src.width, dst.width);
        copy_region.extent.height = std::min(src.height, dst.height);
        copy_region.extent.depth = 1;
        vkCmdCopyImage(m_CommandBuffer, reinterpret_cast<VkImage>(src.resource), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       reinterpret_cast<VkImage>(dst.resource), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
    }

    void VulkanCopyContext::copy_buffer_to_texture(Texture& dst, Buffer& src, const BufferTextureCopy& region) {
        VkBufferImageCopy copy_region{};
        copy_region.bufferOffset = region.buffer_offset;
        copy_region.bufferRowLength = region.buffer_row_length;
        copy_region.bufferImageHeight = region.buffer_image_height;
        copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.imageSubresource.mipLevel = region.mip_level;
        copy_region.imageSubresource.baseArrayLayer = region.base_array_layer;
        copy_region.imageSubresource.layerCount = region.layer_count;
        copy_region.imageOffset = {static_cast<i32>(region.texture_offset.x), static_cast<i32>(region.texture_offset.y),
                                   static_cast<i32>(region.texture_offset.z)};
        copy_region.imageExtent = {region.texture_extent.width, region.texture_extent.height, region.texture_extent.depth};
        vkCmdCopyBufferToImage(m_CommandBuffer, reinterpret_cast<VkBuffer>(src.resource), reinterpret_cast<VkImage>(dst.resource),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
    }

    void VulkanCopyContext::copy_texture_to_buffer(Buffer& dst, Texture& src, const BufferTextureCopy& region) {
        VkBufferImageCopy copy_region{};
        copy_region.bufferOffset = region.buffer_offset;
        copy_region.bufferRowLength = region.buffer_row_length;
        copy_region.bufferImageHeight = region.buffer_image_height;
        copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.imageSubresource.mipLevel = region.mip_level;
        copy_region.imageSubresource.baseArrayLayer = region.base_array_layer;
        copy_region.imageSubresource.layerCount = region.layer_count;
        copy_region.imageOffset = {static_cast<i32>(region.texture_offset.x), static_cast<i32>(region.texture_offset.y),
                                   static_cast<i32>(region.texture_offset.z)};
        copy_region.imageExtent = {region.texture_extent.width, region.texture_extent.height, region.texture_extent.depth};
        vkCmdCopyImageToBuffer(m_CommandBuffer, reinterpret_cast<VkImage>(src.resource), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               reinterpret_cast<VkBuffer>(dst.resource), 1, &copy_region);
    }
} // namespace sf::render::vk
