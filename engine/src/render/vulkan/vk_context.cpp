#include "engpch.h"
#include "render/vulkan/vk_context.h"
#include "render/vulkan/vk_graphics_device.h"
#include "render/vulkan/vk_type_conversions.h"
#include "core/logger.h"

namespace sf::render::vk {

// ============================================================================
// Base VkContext
// ============================================================================

void VkContext::reset() {
    m_ImageBarriers.clear();
    m_BufferBarriers.clear();

    if (m_CommandBuffer != VK_NULL_HANDLE) {
        vkResetCommandBuffer(m_CommandBuffer, 0);
    }
}

void VkContext::close() {
    if (m_CommandBuffer != VK_NULL_HANDLE) {
        vkEndCommandBuffer(m_CommandBuffer);
    }
}

void VkContext::add_resource_barrier(const ResourceBarrier& barrier) {
    CORE_WARN("VkContext::add_resource_barrier - stub implementation");
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
        vkCmdPipelineBarrier(
            m_CommandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            static_cast<u32>(m_BufferBarriers.size()), m_BufferBarriers.data(),
            static_cast<u32>(m_ImageBarriers.size()), m_ImageBarriers.data()
        );

        m_BufferBarriers.clear();
        m_ImageBarriers.clear();
    }
}

// ============================================================================
// VkGraphicsContext
// ============================================================================

VkGraphicsContext::VkGraphicsContext(VkGraphicsDevice* device)
    : m_DevicePtr(device) {
    m_Device = device->get_vk_device();
}

void VkGraphicsContext::reset() {
    // End any active render pass
    if (m_CurrentRenderPass != VK_NULL_HANDLE) {
        vkCmdEndRenderPass(m_CommandBuffer);
        m_CurrentRenderPass = VK_NULL_HANDLE;
        m_CurrentFramebuffer = VK_NULL_HANDLE;
    }

    VkContext::reset();

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(m_CommandBuffer, &begin_info);
}

void VkGraphicsContext::close() {
    // End any active render pass before closing command buffer
    if (m_CurrentRenderPass != VK_NULL_HANDLE) {
        vkCmdEndRenderPass(m_CommandBuffer);
        m_CurrentRenderPass = VK_NULL_HANDLE;
        m_CurrentFramebuffer = VK_NULL_HANDLE;
    }

    VkContext::close();
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
    CORE_WARN("VkGraphicsContext::set_root_signature - stub implementation");
}

void VkGraphicsContext::set_32_bit_constants(const void* data, u32 num_32bit_values, u32 offset) {
    CORE_WARN("VkGraphicsContext::set_32_bit_constants - stub implementation");
}

void VkGraphicsContext::set_descriptor_heaps() {
    CORE_WARN("VkGraphicsContext::set_descriptor_heaps - stub implementation");
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
    // End current render pass if active
    if (m_CurrentRenderPass != VK_NULL_HANDLE) {
        vkCmdEndRenderPass(m_CommandBuffer);
        m_CurrentRenderPass = VK_NULL_HANDLE;
        m_CurrentFramebuffer = VK_NULL_HANDLE;
    }

    // For swapchain rendering, get the framebuffer from the device
    // This is a simplified version that works with the main render pass
    VkRenderPass render_pass = m_DevicePtr->get_main_render_pass();

    // Get the current back buffer index to find the right framebuffer
    u32 back_buffer_index = m_DevicePtr->get_current_back_buffer_index();
    VkFramebuffer framebuffer = m_DevicePtr->get_vk_swapchain_framebuffer(back_buffer_index);

    if (framebuffer == VK_NULL_HANDLE) {
        CORE_ERROR("Invalid framebuffer for render target");
        return;
    }

    // Begin render pass
    VkRenderPassBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = render_pass;
    begin_info.framebuffer = framebuffer;
    begin_info.renderArea.offset = {0, 0};
    begin_info.renderArea.extent = {render_target.width, render_target.height};

    // Clear values (optional, can be set by user before beginning)
    VkClearValue clear_value{};
    clear_value.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
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
    CORE_WARN("VkGraphicsContext::set_primitive_topology - stub implementation");
}

void VkGraphicsContext::draw(u32 vertex_count, u32 instance_count, u32 start_vertex, u32 start_instance) {
    vkCmdDraw(m_CommandBuffer, vertex_count, instance_count, start_vertex, start_instance);
}

void VkGraphicsContext::draw_indexed(u32 index_count, u32 instance_count, u32 start_index, i32 base_vertex, u32 start_instance) {
    vkCmdDrawIndexed(m_CommandBuffer, index_count, instance_count, start_index, base_vertex, start_instance);
}

void VkGraphicsContext::draw_indexed_instanced(u32 index_count_per_instance, u32 instance_count, u32 start_index, i32 base_vertex, u32 start_instance) {
    draw_indexed(index_count_per_instance, instance_count, start_index, base_vertex, start_instance);
}

// ============================================================================
// VkComputeContext
// ============================================================================

VkComputeContext::VkComputeContext(VkGraphicsDevice* device)
    : m_DevicePtr(device) {
    m_Device = device->get_vk_device();
}

void VkComputeContext::reset() {
    VkContext::reset();

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(m_CommandBuffer, &begin_info);
}

void VkComputeContext::set_pipeline_state(IPipelineState* pipeline) {
    if (pipeline) {
        auto* vk_pipeline = static_cast<VkPipelineState*>(pipeline);
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vk_pipeline->get_vk_pipeline());
    }
}

void VkComputeContext::set_root_signature() {
    CORE_WARN("VkComputeContext::set_root_signature - stub implementation");
}

void VkComputeContext::set_32_bit_constants(const void* data, u32 num_32bit_values, u32 offset) {
    CORE_WARN("VkComputeContext::set_32_bit_constants - stub implementation");
}

void VkComputeContext::set_descriptor_heaps() {
    CORE_WARN("VkComputeContext::set_descriptor_heaps - stub implementation");
}

void VkComputeContext::dispatch(u32 thread_group_count_x, u32 thread_group_count_y, u32 thread_group_count_z) {
    vkCmdDispatch(m_CommandBuffer, thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

// ============================================================================
// VkCopyContext
// ============================================================================

VkCopyContext::VkCopyContext(VkGraphicsDevice* device)
    : m_DevicePtr(device) {
    m_Device = device->get_vk_device();
}

void VkCopyContext::reset() {
    VkContext::reset();

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(m_CommandBuffer, &begin_info);
}

void VkCopyContext::copy_buffer(Buffer& dst, Buffer& src, u64 size, u64 dst_offset, u64 src_offset) {
    VkBufferCopy copy_region{};
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;

    vkCmdCopyBuffer(m_CommandBuffer,
                   reinterpret_cast<VkBuffer>(src.resource),
                   reinterpret_cast<VkBuffer>(dst.resource),
                   1, &copy_region);
}

void VkCopyContext::copy_texture(Texture& dst, Texture& src) {
    CORE_WARN("VkCopyContext::copy_texture - stub implementation");
}

void VkCopyContext::copy_buffer_to_texture(Texture& dst, Buffer& src, u32 subresource) {
    CORE_WARN("VkCopyContext::copy_buffer_to_texture - stub implementation");
}

} // namespace sf::render::vk
