#include "engpch.h"

#include <vulkan/vulkan_core.h>
#include "core/logger.h"
#include "render/vulkan/vk_compat.h"
#include "render/vulkan/vk_context.h"
#include "render/vulkan/vk_graphics_device.h"
#include "render/vulkan/vk_type_conversions.h"

namespace sf::render::vk {
    // Base VkContext

    VkContext::VkContext(VkGraphicsDevice* device, u32 family_index, VkCommandPoolCreateFlags pool_ci_flags) : m_Device(nullptr) {
        init(device, family_index, pool_ci_flags);
    }

    VkContext::~VkContext() {
        if (m_Device == VK_NULL_HANDLE)
            return;
        destroy_resources();
    }

    void VkContext::destroy_resources() {
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

    stl::result<> VkContext::init(VkGraphicsDevice* device, u32 family_index, VkCommandPoolCreateFlags pool_ci_flags) {
        m_Device = device->get_vk_device();
        VkCommandPoolCreateInfo command_pool_ci{};
        command_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_ci.pNext = nullptr;
        command_pool_ci.flags = pool_ci_flags;
        command_pool_ci.queueFamilyIndex = family_index;
        VK_RETURN_ON_ERROR(vkCreateCommandPool(m_Device, &command_pool_ci, nullptr, &m_CommandPool), "Failed to create command pool.");
        ;
        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.commandBufferCount = 1;
        alloc_info.commandPool = m_CommandPool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        VK_RETURN_ON_ERROR(vkAllocateCommandBuffers(m_Device, &alloc_info, &m_CommandBuffer), "Failed to allocate command buffer.");
        return stl::success;
    }

    stl::result<> VkContext::reset() {
        m_ImageBarriers.clear();
        m_BufferBarriers.clear();
        if (m_CommandBuffer != VK_NULL_HANDLE) {
            VK_RETURN_ON_ERROR(vkResetCommandBuffer(m_CommandBuffer, 0), "Failed to reset command buffer in VkContext.");
        }
        return stl::success;
    }

    stl::result<> VkContext::close() {
        if (m_CommandBuffer != VK_NULL_HANDLE) {
            VK_RETURN_ON_ERROR(vkEndCommandBuffer(m_CommandBuffer), "Failed to end command buffer in VkContext.");
        }
        return stl::success;
    }

    void VkContext::add_resource_barrier(const ResourceBarrier& barrier) {
        // Generic resource barrier - needs to determine if it's a buffer or texture
        // This is difficult with the current API design since we only have a void* resource
        // Users should prefer using the typed transition_barrier methods
        CORE_WARN("VkContext::add_resource_barrier - generic barriers not fully supported. Use typed transition_barrier methods instead.");
        // For now, assume it's an image barrier (most common case)
        // In production code, you'd need resource type tracking or a different API design
        VkImageMemoryBarrier image_barrier{};
        image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_barrier.oldLayout = to_vk_image_layout(barrier.state_before);
        image_barrier.newLayout = to_vk_image_layout(barrier.state_after);
        image_barrier.srcAccessMask = to_vk_access_flags(barrier.state_before);
        image_barrier.dstAccessMask = to_vk_access_flags(barrier.state_after);
        image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_barrier.image = reinterpret_cast<VkImage>(barrier.resource);
        image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_barrier.subresourceRange.baseMipLevel = 0;
        image_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        image_barrier.subresourceRange.baseArrayLayer = 0;
        image_barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        m_ImageBarriers.push_back(image_barrier);
    }

    void VkContext::transition_barrier(Buffer& buffer, ResourceState before, ResourceState after) {
        VkBufferMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = to_vk_access_flags(before);
        barrier.dstAccessMask = to_vk_access_flags(after);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = reinterpret_cast<VkBuffer>(buffer.resource);
        barrier.offset = 0;
        barrier.size = buffer.size_in_bytes;
        m_BufferBarriers.push_back(barrier);
    }

    void VkContext::transition_barrier(Texture& texture, ResourceState before, ResourceState after) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = to_vk_image_layout(before);
        barrier.newLayout = to_vk_image_layout(after);
        barrier.srcAccessMask = to_vk_access_flags(before);
        barrier.dstAccessMask = to_vk_access_flags(after);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = reinterpret_cast<VkImage>(texture.resource);
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        m_ImageBarriers.push_back(barrier);
    }

    void VkContext::execute_resource_barriers() {
        if (!m_ImageBarriers.empty() || !m_BufferBarriers.empty()) {
            vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr,
                                 static_cast<u32>(m_BufferBarriers.size()), m_BufferBarriers.data(),
                                 static_cast<u32>(m_ImageBarriers.size()), m_ImageBarriers.data());
            m_BufferBarriers.clear();
            m_ImageBarriers.clear();
        }
    }

    // VkGraphicsContext
    VkGraphicsContext::VkGraphicsContext(VkGraphicsDevice* device) :
        VkContext(device, device->get_graphics_queue_family_index(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT), m_DevicePtr(device) {
    }

    stl::result<> VkGraphicsContext::reset() {
        if (m_CurrentRenderPass != VK_NULL_HANDLE) {
            vkCmdEndRenderPass(m_CommandBuffer);
            m_CurrentRenderPass = VK_NULL_HANDLE;
            m_CurrentFramebuffer = VK_NULL_HANDLE;
        }
        VkContext::reset();
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_RETURN_ON_ERROR(vkBeginCommandBuffer(m_CommandBuffer, &begin_info), "Failed to begin command buffer in VkGraphicsContext.");
        return stl::success;
    }

    stl::result<> VkGraphicsContext::close() {
        // End any active render pass before closing command buffer
        if (m_CurrentRenderPass != VK_NULL_HANDLE) {
            vkCmdEndRenderPass(m_CommandBuffer);
            m_CurrentRenderPass = VK_NULL_HANDLE;
            m_CurrentFramebuffer = VK_NULL_HANDLE;
        }
        return VkContext::close();
    }

    void VkGraphicsContext::clear_render_target_view(Texture& texture, stl::span<f32, 4> clear_color) {
        // For clearing within a render pass, use vkCmdClearAttachments
        // This requires the render pass to be active
        if (m_CurrentRenderPass != VK_NULL_HANDLE) {
            VkClearAttachment attachment{};
            attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            attachment.colorAttachment = 0; // Assuming first color attachment
            attachment.clearValue.color.float32[0] = clear_color[0];
            attachment.clearValue.color.float32[1] = clear_color[1];
            attachment.clearValue.color.float32[2] = clear_color[2];
            attachment.clearValue.color.float32[3] = clear_color[3];
            VkClearRect clear_rect{};
            clear_rect.rect.offset = {0, 0};
            clear_rect.rect.extent = {texture.width, texture.height};
            clear_rect.baseArrayLayer = 0;
            clear_rect.layerCount = 1;
            vkCmdClearAttachments(m_CommandBuffer, 1, &attachment, 1, &clear_rect);
        }
    }

    void VkGraphicsContext::clear_depth_stencil_view(Texture& texture, f32 depth, u8 stencil) {
        if (m_CurrentRenderPass != VK_NULL_HANDLE) {
            VkClearAttachment attachment{};
            attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            attachment.clearValue.depthStencil.depth = depth;
            attachment.clearValue.depthStencil.stencil = stencil;
            VkClearRect clear_rect{};
            clear_rect.rect.offset = {0, 0};
            clear_rect.rect.extent = {texture.width, texture.height};
            clear_rect.baseArrayLayer = 0;
            clear_rect.layerCount = 1;
            vkCmdClearAttachments(m_CommandBuffer, 1, &attachment, 1, &clear_rect);
        }
    }

    void VkGraphicsContext::set_pipeline_state(IPipelineState* pipeline) {
        if (pipeline) {
            auto* vk_pipeline = static_cast<VkPipelineState*>(pipeline);
            vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->get_vk_pipeline());
        }
    }

    void VkGraphicsContext::set_root_signature() {
        // In Vulkan, the root signature (pipeline layout) is already bound via the pipeline
        // This function is a no-op in Vulkan as the layout is part of the pipeline state
        // If descriptor sets need to be bound, use set_descriptor_heaps() instead
    }

    void VkGraphicsContext::set_32_bit_constants(const void* data, u32 num_32bit_values, u32 offset) {
        if (num_32bit_values > NUMBER_32_BIT_CONSTANTS) {
            CORE_ERROR("Attempting to set {} 32-bit constants, but max is {}", num_32bit_values, NUMBER_32_BIT_CONSTANTS);
            return;
        }
        VkPipelineLayout layout = m_DevicePtr->get_bindless_pipeline_layout();
        u32 byte_offset = offset * sizeof(u32);
        u32 byte_size = num_32bit_values * sizeof(u32);
        vkCmdPushConstants(m_CommandBuffer, layout, VK_SHADER_STAGE_ALL, byte_offset, byte_size, data);
    }

    void VkGraphicsContext::set_descriptor_heaps() {
        // In Vulkan, this would bind descriptor sets to the pipeline
        // For now, this is a stub as the descriptor heap needs to create and manage descriptor sets
        // When fully implemented, this would call:
        // vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, count, descriptor_sets, 0, nullptr);
        CORE_WARN("VkGraphicsContext::set_descriptor_heaps - descriptor set binding not yet implemented. "
                  "Requires descriptor set allocation and management.");
    }

    void VkGraphicsContext::set_viewport(const Viewport& viewport) {
        VkViewport vk_viewport{};
        vk_viewport.x = viewport.x;
        vk_viewport.y = viewport.y;
        vk_viewport.width = viewport.width;
        vk_viewport.height = viewport.height;
        vk_viewport.minDepth = viewport.min_depth;
        vk_viewport.maxDepth = viewport.max_depth;
        vkCmdSetViewport(m_CommandBuffer, 0, 1, &vk_viewport);
    }

    void VkGraphicsContext::set_scissor_rect(const ScissorRect& scissor) {
        VkRect2D vk_scissor{};
        vk_scissor.offset = {static_cast<i32>(scissor.left), static_cast<i32>(scissor.top)};
        vk_scissor.extent = {static_cast<u32>(scissor.right - scissor.left), static_cast<u32>(scissor.bottom - scissor.top)};
        vkCmdSetScissor(m_CommandBuffer, 0, 1, &vk_scissor);
    }

    void VkGraphicsContext::set_render_target(Texture& render_target, Texture* depth_stencil) {
        if (m_CurrentRenderPass != VK_NULL_HANDLE) {
            vkCmdEndRenderPass(m_CommandBuffer);
            m_CurrentRenderPass = VK_NULL_HANDLE;
            m_CurrentFramebuffer = VK_NULL_HANDLE;
        }
        // This is a simplified version that works with the main render pass
        VkRenderPass render_pass = m_DevicePtr->get_main_render_pass();
        u32 back_buffer_index = m_DevicePtr->get_current_back_buffer_index();
        VkFramebuffer framebuffer = m_DevicePtr->get_vk_swapchain_framebuffer(back_buffer_index);
        if (framebuffer == VK_NULL_HANDLE) {
            CORE_ERROR("Invalid framebuffer for render target");
            return;
        }
        VkRenderPassBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begin_info.renderPass = render_pass;
        begin_info.framebuffer = framebuffer;
        begin_info.renderArea.offset = {0, 0};
        begin_info.renderArea.extent = {render_target.width, render_target.height};
        VkClearValue clear_value{};
        clear_value.color = {{0.1f, 0.1f, 0.1f, 1.0f}};
        begin_info.clearValueCount = 1;
        begin_info.pClearValues = &clear_value;
        vkCmdBeginRenderPass(m_CommandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
        m_CurrentRenderPass = render_pass;
        m_CurrentFramebuffer = framebuffer;
    }

    void VkGraphicsContext::set_render_targets(stl::span<Texture*> render_targets, Texture* depth_stencil) {
        // For now, just use the first render target
        if (!render_targets.empty() && render_targets[0]) {
            set_render_target(*render_targets[0], depth_stencil);
        }
    }

    void VkGraphicsContext::set_index_buffer(Buffer& buffer, Format format) {
        VkIndexType index_type = (format == Format::R16_UINT) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
        vkCmdBindIndexBuffer(m_CommandBuffer, reinterpret_cast<VkBuffer>(buffer.resource), 0, index_type);
    }

    void VkGraphicsContext::set_primitive_topology(PrimitiveTopology topology) {
        // Note: This requires VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY to be enabled
        // in the pipeline creation (VK_EXT_extended_dynamic_state or Vulkan 1.3+)
        // Currently the pipeline is created with fixed topology
        // For Vulkan 1.3 or with VK_EXT_extended_dynamic_state:
        // vkCmdSetPrimitiveTopology(m_CommandBuffer, to_vk_primitive_topology(topology));
        // For now, log a warning as the dynamic state is not enabled
        CORE_WARN("VkGraphicsContext::set_primitive_topology - requires VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY. "
                  "Topology should be set during pipeline creation instead.");
    }

    void VkGraphicsContext::draw(u32 vertex_count, u32 instance_count, u32 start_vertex, u32 start_instance) {
        vkCmdDraw(m_CommandBuffer, vertex_count, instance_count, start_vertex, start_instance);
    }

    void VkGraphicsContext::draw_indexed(u32 index_count, u32 instance_count, u32 start_index, i32 base_vertex, u32 start_instance) {
        vkCmdDrawIndexed(m_CommandBuffer, index_count, instance_count, start_index, base_vertex, start_instance);
    }

    void VkGraphicsContext::draw_indexed_instanced(u32 index_count_per_instance, u32 instance_count, u32 start_index, i32 base_vertex,
                                                   u32 start_instance) {
        draw_indexed(index_count_per_instance, instance_count, start_index, base_vertex, start_instance);
    }

    // VkComputeContext
    VkComputeContext::VkComputeContext(VkGraphicsDevice* device) :
        VkContext(device, device->get_compute_queue_family_index(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT), m_DevicePtr(device) {}

    stl::result<> VkComputeContext::reset() {
        VkContext::reset();
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_RETURN_ON_ERROR(vkBeginCommandBuffer(m_CommandBuffer, &begin_info), "Failed to begin command buffer in VkComputeContext.");
        return stl::success;
    }

    void VkComputeContext::set_pipeline_state(IPipelineState* pipeline) {
        if (pipeline) {
            auto* vk_pipeline = static_cast<VkPipelineState*>(pipeline);
            vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vk_pipeline->get_vk_pipeline());
        }
    }

    void VkComputeContext::set_root_signature() {
        // In Vulkan, the root signature (pipeline layout) is already bound via the pipeline
        // This function is a no-op in Vulkan as the layout is part of the pipeline state
        // If descriptor sets need to be bound, use set_descriptor_heaps() instead
    }

    void VkComputeContext::set_32_bit_constants(const void* data, u32 num_32bit_values, u32 offset) {
        if (num_32bit_values > NUMBER_32_BIT_CONSTANTS) {
            CORE_ERROR("Attempting to set {} 32-bit constants, but max is {}", num_32bit_values, NUMBER_32_BIT_CONSTANTS);
            return;
        }
        VkPipelineLayout layout = m_DevicePtr->get_bindless_pipeline_layout();
        u32 byte_offset = offset * sizeof(u32);
        u32 byte_size = num_32bit_values * sizeof(u32);
        vkCmdPushConstants(m_CommandBuffer, layout, VK_SHADER_STAGE_ALL, byte_offset, byte_size, data);
    }

    void VkComputeContext::set_descriptor_heaps() {
        // In Vulkan, this would bind descriptor sets to the pipeline
        // For now, this is a stub as the descriptor heap needs to create and manage descriptor sets
        // When fully implemented, this would call:
        // vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, count, descriptor_sets, 0, nullptr);
        CORE_WARN("VkComputeContext::set_descriptor_heaps - descriptor set binding not yet implemented. "
                  "Requires descriptor set allocation and management.");
    }

    void VkComputeContext::dispatch(u32 thread_group_count_x, u32 thread_group_count_y, u32 thread_group_count_z) {
        vkCmdDispatch(m_CommandBuffer, thread_group_count_x, thread_group_count_y, thread_group_count_z);
    }

    // VkCopyContext
    VkCopyContext::VkCopyContext(VkGraphicsDevice* device) :
        VkContext(device, device->get_transfer_queue_family_index(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT), m_DevicePtr(device) {
    }

    stl::result<> VkCopyContext::reset() {
        VkContext::reset();
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_RETURN_ON_ERROR(vkBeginCommandBuffer(m_CommandBuffer, &begin_info), "Failed to begin command buffer in VkCopyContext.");
        return stl::success;
    }

    void VkCopyContext::copy_buffer(Buffer& dst, Buffer& src, u64 size, u64 dst_offset, u64 src_offset) {
        VkBufferCopy copy_region{};
        copy_region.srcOffset = src_offset;
        copy_region.dstOffset = dst_offset;
        copy_region.size = size;
        vkCmdCopyBuffer(m_CommandBuffer, reinterpret_cast<VkBuffer>(src.resource), reinterpret_cast<VkBuffer>(dst.resource), 1,
                        &copy_region);
    }

    void VkCopyContext::copy_texture(Texture& dst, Texture& src) {
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

    void VkCopyContext::copy_buffer_to_texture(Texture& dst, Buffer& src, u32 subresource) {
        VkBufferImageCopy copy_region{};
        copy_region.bufferOffset = 0;
        copy_region.bufferRowLength = 0; // Tightly packed
        copy_region.bufferImageHeight = 0; // Tightly packed
        copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.imageSubresource.mipLevel = subresource;
        copy_region.imageSubresource.baseArrayLayer = 0;
        copy_region.imageSubresource.layerCount = 1;
        copy_region.imageOffset = {0, 0, 0};
        copy_region.imageExtent = {dst.width, dst.height, 1};
        vkCmdCopyBufferToImage(m_CommandBuffer, reinterpret_cast<VkBuffer>(src.resource), reinterpret_cast<VkImage>(dst.resource),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
    }
} // namespace sf::render::vk
